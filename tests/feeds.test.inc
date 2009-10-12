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
   * Debug utility. Shows current screen.
   */
  public function show() {
    // Use error, this will make sure we don't forget accidental calls to show()
    // in our tests.
    $this->error($this->content);
  }

  /**
   * Create feed config.
   *
   * @param $name
   *   The natural name of the feed.
   * @param $id
   *   The persistent id of the feed.
   * @param $edit
   *   Optional array that defines the basic settings for the feed in a format
   *   that can be posted to the feed's basic settings form.
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
  public function createFeedNode($id = 'syndication', $title = NULL, $feed_url = NULL) {
    if (empty($title)) {
      $title = 'Development Seed';
    }
    if (empty($feed_url)) {
      $feed_url = $GLOBALS['base_url'] .'/'. drupal_get_path('module', 'feeds') .'/tests/feeds/developmentseed.rss2';
    }
    // Ask the configuration for the content type to create.
    $config = unserialize(db_result(db_query('SELECT config FROM {feeds_config} WHERE id = "%s"', $id)));
    $content_type = $config['content_type'];
    $this->assertFalse(empty($content_type), 'Valid content type found: '. $content_type);

    // Create a feed node.
    $edit = array(
      'title' => $title,
      'feeds[FeedsHttpFetcher][url]' => $feed_url,
    );
    $this->drupalPost('node/add/'. $content_type, $edit, 'Save');
    $this->assertText('Page '. $title .' has been created.');

    // Get the node id from URL.
    $url = $this->getUrl();
    $matches = array();
    preg_match('/node\/(\d+?)$/', $this->getUrl(), $matches);
    $nid = $matches[1];
    $this->assertTrue(is_numeric($nid), 'Found node id ('. $nid .').');

    // Check whether feed got recorded in feeds_source table.
    $this->assertEqual(1, db_result(db_query('select COUNT(*) from {feeds_source} WHERE id = "%s" AND feed_nid = %d', $id, $nid)));
    $source = db_fetch_object(db_query('select * from {feeds_source} WHERE id = "%s" AND feed_nid = %d', $id, $nid));
    $config = unserialize($source->config);
    $this->assertEqual($config['FeedsHttpFetcher']['url'], $feed_url, t('URL in DB correct.'));

    // Check whether feed got properly added to scheduler.
    $this->assertEqual(1, db_result(db_query('select COUNT(*) from {feeds_schedule} WHERE id = "%s" AND feed_nid = %d', $id, $nid)));
    $schedule = db_fetch_object(db_query('select * from {feeds_schedule} WHERE id = "%s" AND feed_nid = %d', $id, $nid));
    $this->assertEqual($schedule->scheduled, 0, 'Feed is not set scheduled.');
    $this->assertEqual($schedule->last_scheduled_time, 0, 'Last scheduled time is 0.');

    return $nid;
  }

  /**
   * Batch create a variable amount of feed nodes. All will have the
   * same URL configured.
   *
   * @return
   *   An array of node ids of the nodes created.
   */
  public function createFeedNodes($id = 'syndication', $num = 20) {
    $nids = array();
    for ($i = 0; $i < $num; $i++) {
      $nids[] = $this->createFeedNode($id, $this->randomName());
    }
    return $nids;
  }

  /**
   * Import a URL through the import form. Assumes FeedsHttpFetcher in place.
   */
  public function importURL($id, $feed_url = NULL) {
    if (empty($feed_url)) {
      $feed_url = $GLOBALS['base_url'] .'/'. drupal_get_path('module', 'feeds') .'/tests/feeds/developmentseed.rss2';
    }
    $edit = array(
      'feeds[FeedsHttpFetcher][url]' => $feed_url,
      );
    $nid = $this->drupalPost('import/'. $id, $edit, 'Import');

    // Check whether feed got recorded in feeds_source table.
    $this->assertEqual(1, db_result(db_query('select COUNT(*) from {feeds_source} WHERE id = "%s" AND feed_nid = 0', $id)));
    $source = db_fetch_object(db_query('select * from {feeds_source} WHERE id = "%s" AND feed_nid = 0', $id));
    $config = unserialize($source->config);
    $this->assertEqual($config['FeedsHttpFetcher']['url'], $feed_url, t('URL in DB correct.'));

    // Check whether feed got properly added to scheduler.
    $this->assertEqual(1, db_result(db_query('select COUNT(*) from {feeds_schedule} WHERE id = "%s" AND feed_nid = 0', $id)));
    $schedule = db_fetch_object(db_query('select * from {feeds_schedule} WHERE id = "%s" AND feed_nid = 0', $id));
    $this->assertEqual($schedule->scheduled, 0, 'Feed is not set scheduled.');
    $this->assertEqual($schedule->last_scheduled_time, 0, 'Last scheduled time is 0.');
  }

  /**
   * Assert a feeds configuration's plugins.
   */
  public function assertPlugins($id, $fetcher, $parser, $processor) {
    // Assert actual configuration.
    $config = unserialize(db_result(db_query('SELECT config FROM {feeds_config} WHERE id = "%s"', $id)));

    $this->assertEqual($config['fetcher']['plugin_key'], $fetcher, 'Correct fetcher');
    $this->assertEqual($config['parser']['plugin_key'], $parser, 'Correct parser');
    $this->assertEqual($config['processor']['plugin_key'], $processor, 'Correct processor');
  }

  /**
   * Add mappings to a given configuration.
   *
   * @param $mappings
   *   An array of mapping arrays. Each mapping array must have a source and
   *   an target key and can have a unique key.
   *
   * @see FeedsRSStoDataTest class.
   */
  public function addMappings($id, $mappings) {

    $path = 'admin/build/feeds/edit/'. $id .'/mapping';

    // Iterate through all mappings and add the via the form.
    foreach ($mappings as $i => $mapping) {

      // Get unique flag and unset it - otherwise drupalPost will complain that
      // there is no form element named "unique".
      $unique = !empty($mapping['unique']);
      unset($mapping['unique']);
      $this->drupalPost($path, $mapping, t('Add'));

      // If unique was set, set the last mapping's unique flag.
      if ($unique) {
        $edit = array(
          'unique_flags['. $i .']' => 1,
        );
        $this->drupalPost($path, $edit, t('Save'));
      }
    }
  }
}