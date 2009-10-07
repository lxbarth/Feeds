<?php
// $Id$

/**
 * @file
 * Common functionality for all Feeds tests.
 */

/**
 * Test basic Data API functionality.
 */
class FeedsWebTestCase extends DrupalWebTestCase {

  /**
   * Create feed config.
   */
  public function createFeedConfiguration($name = 'Syndication', $id = 'syndication') {
    // Create new feed configuration.
    $this->drupalGet('admin/build/feeds');
    $this->clickLink('New configuration');
    $edit = array(
      'name' => $name,
      'id' => $id,
    );
    $this->drupalPost('admin/build/feeds/create', $edit, 'Create');

    // Assert message and presence of default plugins.
    $this->assertText('Your feed has been created with default settings.');
    $this->assertPlugins($id, 'FeedsHttpFetcher', 'FeedsSyndicationParser', 'FeedsNodeProcessor');
  }

  /**
   * Create a test feed node. Test user has to have sufficient permissions:
   *
   * * create [type] content
   * * use feeds
   *
   * Assumes that page content type has been configured with createFeedConfiguration()
   * as a feed content type.
   *
   * @return
   *   The node id of the node created.
   */
  public function createFeedNode($title = NULL, $feed_url = NULL) {
    if (empty($title)) {
      $title = 'Development Seed';
    }
    if (empty($feed_url)) {
      $feed_url = $GLOBALS['base_url'] .'/'. drupal_get_path('module', 'feeds') .'/tests/feeds/developmentseed.rss2';
    }
    // Create a feed node.
    $edit = array(
      'title' => $title,
      'feeds[FeedsHttpFetcher][url]' => $feed_url,
    );
    $this->drupalPost('node/add/page', $edit, 'Save');
    $this->assertText('Page '. $title .' has been created.');

    // Get the node id from URL.
    $url = $this->getUrl();
    $matches = array();
    preg_match('/node\/(\d+?)$/', $this->getUrl(), $matches);
    $nid = $matches[1];
    $this->assertTrue(is_numeric($nid), 'Found node id ('. $nid .').');

    // Check whether feed got recorded in feeds_source table.
    $this->assertEqual(1, db_result(db_query('select COUNT(*) from {feeds_source} WHERE id = "syndication" AND feed_nid = %d', $nid)));
    $source = db_fetch_object(db_query('select * from {feeds_source} WHERE id = "syndication" AND feed_nid = %d', $nid));
    $config = unserialize($source->config);
    $this->assertEqual($config['FeedsHttpFetcher']['url'], $feed_url, t('URL in DB correct.'));

    // Check whether feed got properly added to scheduler.
    $this->assertEqual(1, db_result(db_query('select COUNT(*) from {feeds_schedule} WHERE id = "syndication" AND feed_nid = %d', $nid)));
    $schedule = db_fetch_object(db_query('select * from {feeds_schedule} WHERE id = "syndication" AND feed_nid = %d', $nid));
    $this->assert($schedule->next_refresh_time > time(), 'Next refresh time is in future.');

    return $nid;
  }

  /**
   * Batch create a variable amount of feed nodes. All will have the
   * same URL configured.
   *
   * @return
   *   An array of node ids of the nodes created.
   */
  public function createFeedNodes($num) {
    $nids = array();
    for ($i = 0; $i < $num; $i++) {
      $nids[] = $this->createFeedNode($this->randomName());
    }
    return $nids;
  }

  /**
   * Assert a feeds configuration's plugins.
   */
  public function assertPlugins($id, $fetcher, $parser, $processor) {
    // Assert actual configuration.
    $result = db_query('SELECT * FROM {feeds_config} WHERE id = "%s"', $id);
    $config = array();
    while ($row = db_fetch_object($result)) {
      $config[$row->class] = unserialize($row->config);
    }
    $this->assertEqual($config['Feed']['fetcher']['plugin_key'], $fetcher, 'Correct fetcher');
    $this->assertEqual($config['Feed']['parser']['plugin_key'], $parser, 'Correct parser');
    $this->assertEqual($config['Feed']['processor']['plugin_key'], $processor, 'Correct processor');
  }
}