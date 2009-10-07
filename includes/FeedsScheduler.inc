<?php
// $Id$

/**
 * @file
 * FeedsScheduler class and related.
 */

/**
 * Describe a scheduler.
 */
interface IFeedsScheduler {
  /**
   * Run Drupal cron.
   */
  public function cron();
  /**
   * Add a feed to the schedule.
   *
   * @param $feed_id
   *   Id of a Feed object.
   * @param $feed_nid
   *   Feed nid that identifies the source for this configuration.
   */
  public function add($feed_id, $feed_nid = 0);
  /**
   * Remove a feed from the schedule.
   *
   * @param $feed_id
   *   Id of a Feed object.
   * @param $feed_nid
   *   Feed nid that identifies the source for this configuration.
   */
  public function remove($feed_id, $feed_nid = 0);
  /**
   * Work off a given feed identified by $feed_info.
   *
   * @param $feed_info
   *   Array where 'feed_id' key is the id of a Feed object,
   *   and 'feed_nid' is the feed node id that identifies the
   *   source of a FeedsSource object.
   */
  public function work($feed_info);
}

/**
 * Implementation of IFeedsScheduler.
 *
 * This scheduler uses the last_scheduled_time paradigm: By storing when a
 * particular feed was scheduled to be refreshd last rather than storing
 * when a feed should be _refreshed_ next, we gain two advantages:
 *
 * 1) If a feed's refresh_period setting changes, it has immediate effects,
 *    without batch updating an existing schedule.
 * 2) The time between refreshs will always be scheduled based on when it
 *    has been scheduled last. Less drift occurs.
 */
class FeedsScheduler implements IFeedsScheduler {

  // Only used for debugging.
  protected $debugTime;

  /**
   * Create a single instance of FeedsScheduler.
   */
  public static function instance() {
    static $instance;
    if (!isset($instance)) {
      $class = variable_get('feeds_scheduler_class', 'FeedsScheduler');
      $instance = new $class();
    }
    return $instance;
  }

  /**
   * Protect constructor.
   */
  protected function __construct() {}

  /**
   * Implementation of IFeedsScheduler::cron().
   *
   * Refreshes scheduled feeds.
   *
   * If drupal_queue is present, only moves refresh tasks to drupal queue and
   * returns. If drupal_queue is not available, works off tasks.
   */
  public function cron() {

    // Logging and timeout tracking.
    if ($this->time() == FEEDS_REQUEST_TIME) {
      watchdog('FeedsScheduler', 'Start processing schedule - FEEDS_REQUEST_TIME !date !time !', array('!date' => format_date($this->time()), '!time' => $this->time()));
    }
    else {
      watchdog('FeedsScheduler', 'Start processing schedule - debug time !date !time !', array('!date' => format_date($this->time()), '!time' => $this->time()));
    }
    if (variable_get('feeds_scheduler_cron', FALSE)) {
      watchdog('FeedsScheduler', 'Last cron process did not finish.', array(), WATCHDOG_ERROR);
    }
    variable_set('feeds_scheduler_cron', TRUE);

    // Get feeds configuration, check whether drupal_queue is present and set parameters accordingly.
    if ($feeds = feeds_load_all()) {
      $use_queue = module_exists('drupal_queue');
      $num = variable_get('feeds_schedule_num', $use_queue ? 200 / count($feeds) : 10 / count($feeds));
      $num = $num ? $num : 1;

      // Iterate over feed configurations, push to queue or refresh feeds.
      foreach ($feeds as $feed) {
        if ($feed->config['refresh_period'] != FEEDS_SCHEDULE_NEVER) {

          // Refresh feeds that have a refresh time older than now - refresh period.
          $time = $this->time() - $feed->config['refresh_period'];
          $result = db_query_range('SELECT feed_nid, id AS feed_id FROM {feeds_schedule} WHERE scheduled = 0 AND last_scheduled_time > %d ORDER BY last_scheduled_time DESC', $time, 0, $num);
          while ($feed_info = db_fetch_array($result)) {

            // If drupal_queue is present, add to queue, otherwise work off immediately.
            if ($use_queue) {
              if ($queue->createItem($feed_info)) {
                $this->mark($feed_info['feed_nid'], 1);
              }
            }
            else {
              $this->mark($feed_info['feed_nid'], 1);
              $this->work($feed_info);
            }
          }
        }
      }
    }

    // Post a message that we're done.
    watchdog('FeedsScheduler', 'Finished processing schedule after !time.', format_interval(array('!time' => time() - $this->time())));
  }

  /**
   * Implementation of IFeedsScheduler::add().
   *
   * Add a feed to schedule if its refresh_period is set.
   *
   * @todo: better name as this function can also remove a feed from the schedule
   * depending on its configuration.
   * @todo: create an abstract interface for items that can be added?
   */
  public function add($feed_id, $feed_nid = 0) {
    $object = array(
      'id' => $feed_id,
      'feed_nid' => $feed_nid,
      'last_scheduled_time' => 0,
      'scheduled' => 0, // Means NOT scheduled at the moment.
    );
    drupal_write_record('feeds_schedule', $object, 'feed_nid');
    if (!db_affected_rows()) {
      drupal_write_record('feeds_schedule', $object);
    }
  }

  /**
   * Implementation of IFeedsScheduler::remove().
   */
  public function remove($feed_nid = 0) {
    db_query('DELETE FROM {feeds_schedule} WHERE feed_nid = %d', $feed_nid);
  }

  /**
   * Implementation of IFeedsScheduler::work().
   *
   * Refresh a feed.
   *
   * Used as worker callback invoked from feeds_scheduler_refresh() or
   * if drupal_queue is not enabled, directly from $this->cron().
   */
  public function work($feed_info) {
    $feed = feeds_feed_instance($feed_info['feed_id']);

    // Only refresh if feed is actually in DB or in default configuration.
    if ($feed->export_type != FEEDS_EXPORT_NONE) {

      // Remove scheduled flag, if we fail after this we'd like to try again next time.
      $this->unmark($feed['feed_nid']);

      try {
        feeds_import($feed, $feed_info['feed_nid']);
      }
      catch (Exception $e) {
        watchdog('feeds_refresh', $e->getMessage(), array(), WATCHDOG_ERROR);
      }
    }
  }

  /**
   * Set the internal time of FeedsScheduler.
   * Use for debugging.
   *
   * @param $time
   *   UNIX time that the scheduler should use for comparing the schedule. Set
   *   this time to test the behavior of the scheduler in the future or past.
   *   If set to 0, FeedsScheduler will use the current time.
   */
  public function debugSetTime($time) {
    $this->debugTime = $time;
  }

  /**
   * Returns the internal time that the scheduler is operating on.
   *
   * Usually returns FEEDS_REQUEST_TIME, unless a debug time has been set
   * with debugSetTime();
   *
   * @return
   *   An integer that is a UNIX time.
   */
  public function time() {
    return empty($this->debugTime) ? FEEDS_REQUEST_TIME : $this->debugTime;
  }

  /**
   * Helper function to mark a feed scheduled.
   *
   * This function sets the feed's scheduled bit to 1 and updates
   * last_scheduled_time to $this->time().
   *
   * @param $feed_nid
   *   Identifier of the feed.
   */
  protected function mark($feed_nid) {
    db_query('UPDATE {feeds_schedule} SET last_scheduled_time = %d AND scheduled = 1 WHERE feed_nid = %d', $this->time(), $feed_nid);
  }

  /**
   * Helper function to mark a feed unscheduled.
   *
   * This function sets the feed's scheduled bit to 0 and thus makes
   * it eligible for being added to the queue again.
   *
   * @param $feed_nid
   *   Identifier of the feed.
   */
  protected function unmark($feed_nid) {
    db_query('UPDATE {feeds_schedule} SET scheduled = 0 WHERE feed_nid = %d', $feed_nid);
  }
}