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

    // Do the actual work. If Drupal Queue module exists, just move tasks to
    // the queue and return. Otherwise pass the task to $this->work().
    if (module_exists('drupal_queue')) {
      $result = db_query('SELECT feed_nid, id FROM {feeds_schedule} WHERE next_refresh_time > %d', $this->time());
      $queue = drupal_queue_get('feeds_queue');
      while ($feed_info = db_fetch_array($result)) {
        if ($queue->createItem($feed_info)) {
          // Remove item from schedule after adding it to queue.
          // This will make sure that we don't add an item twice to the queue.
          // @todo: There is a chance that we remove and never add again. This
          // could happen if PHP dies between the Queue dispatching a task and
          // the worker adding this task again to the queue.
          // @see $this->add() in $this->work().
          // Solution: don't remove feeds but flag them as queued and pick them up manually.
          $this->remove($feed_info['feed_id'], $feed_info['feed_nid']);
        }
      }
    }
    else {
      $result = db_query_range('SELECT feed_nid, id FROM {feeds_schedule} WHERE next_refresh_time > %d', $this->time(), 0, 5);
      while ($feed_info = db_fetch_array($result)) {
        $this->work($feed_info);
      }
    }
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
    $feed = feeds_feed_instance($feed_id);

    // Remove if the configuration of a feed says it should be never refreshed.
    if ($feed->config['refresh_period'] == FEEDS_SCHEDULE_NEVER_REFRESH) {
      $this->remove($feed->id, $feed_nid);
    }
    else {
      $next_refresh = $this->time() + $feed->config['refresh_period'];
      db_query('UPDATE {feeds_schedule} SET id = "%s", feed_nid = %d, next_refresh_time = %d',  $feed->id, $feed_nid, $next_refresh);
      if (!db_affected_rows()) {
        db_query('INSERT INTO {feeds_schedule}(id, feed_nid, next_refresh_time) VALUES("%s", %d, %d)',  $feed->id, $feed_nid, $next_refresh);
      }
    }
  }

  /**
   * Implementation of IFeedsScheduler::remove().
   */
  public function remove($feed_id, $feed_nid = 0) {
    db_query('DELETE FROM {feeds_schedule} WHERE id = "%s" AND feed_nid = %d', $feed_id, $feed_nid);
  }

  /**
   * Implementation of IFeedsScheduler::work().
   *
   * Refresh a feed. Worker callback invoked from feeds_scheduler_refresh().
   */
  public function work($feed_info) {
    $feed = feeds_feed_instance($feed_info['id']);

    // Only refresh if feed is actually in DB or in default configuration,
    // otherwise remove from schedule.
    if ($feed->export_type == FEEDS_EXPORT_NONE) {
      $this->remove($feed, $feed_info['feed_nid']);
    }
    else {

      // Add feed to scheduler no matter if successfully worked off or not.
      // This serves as a protection from feeds that don't return before
      // script timeout.
      //
      // @todo: reschedule stuck feeds earlier than their refresh_period
      // setting.
      //
      // @todo: right now, we are setting the next refresh time to now +
      // refresh period. This could drift badly as feeds are getting slower
      // and slower updated. better would be
      // last_scheduled_time + refresh_period
      $this->add($feed, $feed_info['feed_nid']);

      try {
        feeds_import($feed, $feed_info['feed_nid']);
      }
      catch (Exception $e) {
        // If source is gone, remove feed from schedule.
        if (get_class($e) == 'FeedsNoSourceException') {
          feeds_scheduler_remove($feed, $feed_info['feed_nid']);
          watchdog('feeds_refresh', 'Removed '. implode('/'. $feed_info .' from schedule.'), array(), WATCHDOG_NOTICE);
        }
        // Log all other errors in a generic way.
        else {
          watchdog('feeds_refresh', $e->getMessage(), array(), WATCHDOG_ERROR);
        }
      }
    }
  }
}