<?php
// $Id$

/**
 * @file
 * Scheduling refreshing of feeds.
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
    if (module_exists('drupal_queue')) {
      // @todo: test.
      $result = db_query('SELECT feed_nid, id FROM {feeds_schedule} WHERE next_refresh_time > %d', FEEDS_REQUEST_TIME);
      $queue = drupal_queue_get('feeds_queue');
      while ($feed_info = db_fetch_array($result)) {
        if ($queue->createItem($feed_info)) {
          // Remove item from schedule after adding it to queue.
          // This will make sure that we don't add an item twice to the queue.
          $this->remove($feed_info);
        }
      }
    }
    else {
      $result = db_query_range('SELECT feed_nid, id FROM {feeds_schedule} WHERE next_refresh_time > %d', FEEDS_REQUEST_TIME, 0, 5);
      while ($feed_info = db_fetch_array($result)) {
        $this->work($feed_info);
      }
    }
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
      $next_refresh = FEEDS_REQUEST_TIME + $feed->config['refresh_period'];
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
      // @todo: reschedule stuck feeds earlier than their refresh_period
      // setting.
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