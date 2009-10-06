<?php
// $Id: queue.inc,v 1.1 2009/09/16 17:58:13 alexb Exp $

/**
 * @file
 * Scheduling refreshing of feeds.
 */

/**
 * Refreshes scheduled feeds.
 *
 * If drupal_queue is present, only moves refresh tasks to queue and returns.
 * If not, works off tasks.
 */
function feeds_scheduler_cron() {

  // Delete all scheduled entries that don't have a valid source record anymore.
  db_query('DELETE schedule FROM {feeds_schedule} schedule LEFT JOIN {feeds_source} source ON schedule.feed_nid = source.feed_nid WHERE source.feed_nid IS NULL');

  if (module_exists('drupal_queue')) {
    // @todo: test.
    $result = db_query('SELECT feed_nid, id FROM {feeds_schedule} WHERE next_refresh_time > %d', FEEDS_REQUEST_TIME);
    $queue = drupal_queue_get('feeds_queue');
    while ($feed_info = db_fetch_array($result)) {
      $queue->createItem($feed_info);
    }
  }
  else {
    $result = db_query_range('SELECT feed_nid, id FROM {feeds_schedule} WHERE next_refresh_time > %d', FEEDS_REQUEST_TIME, 0, 5);
    while ($feed_info = db_fetch_array($result)) {
      feeds_refresh($feed_info);
    }
  }
}

/**
 * Add a feed to the schedule.
 *
 * @param $feed
 *   Feed object.
 */
function feeds_scheduler_add($feed, $feed_nid = 0) {
  if ($feed->config['refresh_period'] != FEEDS_SCHEDULE_NEVER_REFRESH) {
    $next_refresh = FEEDS_REQUEST_TIME + $feed->config['refresh_period'];
    db_query('UPDATE {feeds_schedule} SET id = "%s", feed_nid = %d, next_refresh_time = %d',  $feed->id, $feed_nid, $next_refresh);
    if (!db_affected_rows()) {
      db_query('INSERT INTO {feeds_schedule}(id, feed_nid, next_refresh_time) VALUES("%s", %d, %d)',  $feed->id, $feed_nid, $next_refresh);
    }
  }
}

/**
 * Remove a feed from the schedule.
 */
function feeds_scheduler_remove($feed) {
  db_query('DELETE FROM {feeds_schedule} WHERE feed_nid = %d, id = "%s"', FEEDS_REQUEST_TIME);
}
