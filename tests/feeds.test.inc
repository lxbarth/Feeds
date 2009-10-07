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
  public function createFeedConfiguration($name, $id) {
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