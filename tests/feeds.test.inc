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
   * Create an importer configuration.
   *
   * @todo: rename to createImporterConfig()
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
    $this->assertText('Your configuration has been created with default settings.');
    $this->assertPlugins($id, 'FeedsHTTPFetcher', 'FeedsSyndicationParser', 'FeedsNodeProcessor');
  }

  /**
   * Choose a plugin for a importer configuration and assert it.
   *
   * @param $id
   *   The importer configuration's id.
   * @param $plugin_key
   *   The key string of the plugin to choose (one of the keys defined in
   *   feeds_feeds_plugins()).
   */
  public function setPlugin($id, $plugin_key) {
    if ($type = feeds_plugin_type($plugin_key)) {
      $edit = array(
        'plugin_key' => $plugin_key,
      );
      $this->drupalPost("admin/build/feeds/edit/$id/$type", $edit, 'Save');

      // Assert actual configuration.
      $config = unserialize(db_result(db_query('SELECT config FROM {feeds_importer} WHERE id = "%s"', $id)));
      $this->assertEqual($config[$type]['plugin_key'], $plugin_key, 'Verified correct '. $type .' ('. $plugin_key .').');
    }
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
  public function createFeedNode($id = 'syndication', $feed_url = NULL, $title = '') {
    if (empty($feed_url)) {
      $feed_url = $GLOBALS['base_url'] .'/'. drupal_get_path('module', 'feeds') .'/tests/feeds/developmentseed.rss2';
    }
    // Ask the configuration for the content type to create.
    $config = unserialize(db_result(db_query('SELECT config FROM {feeds_importer} WHERE id = "%s"', $id)));
    $content_type = $config['content_type'];
    $this->assertFalse(empty($content_type), 'Valid content type found: '. $content_type);

    // Create a feed node.
    $edit = array(
      'title' => $title,
      'feeds[FeedsHTTPFetcher][source]' => $feed_url,
    );
    $this->drupalPost('node/add/'. $content_type, $edit, 'Save');
    $this->assertText('has been created.');

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
    $this->assertEqual($config['FeedsHTTPFetcher']['source'], $feed_url, t('URL in DB correct.'));

    // Check whether feed got properly added to scheduler.
    $this->assertEqual(1, db_result(db_query('SELECT COUNT(*) FROM {feeds_schedule} WHERE id = "%s" AND feed_nid = %d AND callback = "import" AND last_scheduled_time = 0 AND scheduled = 0', $id, $nid)));
    // There must be only one entry for 'expire' - no matter how many actuall feed nodes exist.
    $this->assertEqual(1, db_result(db_query('SELECT COUNT(*) FROM {feeds_schedule} WHERE id = "%s" AND callback = "expire" AND last_scheduled_time = 0 AND scheduled = 0', $id)));

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
      $nids[] = $this->createFeedNode($id, NULL, $this->randomName());
    }
    return $nids;
  }

  /**
   * Import a URL through the import form. Assumes FeedsHTTPFetcher in place.
   */
  public function importURL($id, $feed_url = NULL) {
    if (empty($feed_url)) {
      $feed_url = $GLOBALS['base_url'] .'/'. drupal_get_path('module', 'feeds') .'/tests/feeds/developmentseed.rss2';
    }
    $edit = array(
      'feeds[FeedsHTTPFetcher][source]' => $feed_url,
      );
    $nid = $this->drupalPost('import/'. $id, $edit, 'Import');

    // Check whether feed got recorded in feeds_source table.
    $this->assertEqual(1, db_result(db_query('SELECT COUNT(*) FROM {feeds_source} WHERE id = "%s" AND feed_nid = 0', $id)));
    $source = db_fetch_object(db_query('SELECT * FROM {feeds_source} WHERE id = "%s" AND feed_nid = 0', $id));
    $config = unserialize($source->config);
    $this->assertEqual($config['FeedsHTTPFetcher']['source'], $feed_url, t('URL in DB correct.'));

    // Check whether feed got properly added to scheduler.
    $this->assertEqual(1, db_result(db_query('SELECT COUNT(*) FROM {feeds_schedule} WHERE id = "%s" AND feed_nid = 0 AND callback = "import" AND last_scheduled_time = 0 AND scheduled = 0', $id)));
    // There must be only one entry for callback 'expire' - no matter what the feed_nid is.
    $this->assertEqual(1, db_result(db_query('SELECT COUNT(*) FROM {feeds_schedule} WHERE id = "%s" AND callback = "expire" AND last_scheduled_time = 0 AND scheduled = 0', $id)));
  }

  /**
   * Assert a feeds configuration's plugins.
   *
   * @deprecated:
   *   Use setPlugin() instead.
   *
   * @todo:
   *   Refactor users of assertPlugin() and make them use setPugin() instead.
   */
  public function assertPlugins($id, $fetcher, $parser, $processor) {
    // Assert actual configuration.
    $config = unserialize(db_result(db_query('SELECT config FROM {feeds_importer} WHERE id = "%s"', $id)));

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