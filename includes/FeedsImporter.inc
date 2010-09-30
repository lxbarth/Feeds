<?php
// $Id: FeedsImporter.inc,v 1.31.2.2 2010/09/30 17:45:47 alexb Exp $

/**
 * @file
 * FeedsImporter class and related.
 */

/**
 * A FeedsImporter object describes how an external source should be fetched,
 * parsed and processed. Feeds can manage an arbitrary amount of importers.
 *
 * A FeedsImporter holds a pointer to a FeedsFetcher, a FeedsParser and a
 * FeedsProcessor plugin. It further contains the configuration for itself and
 * each of the three plugins.
 *
 * Its most important responsibilities are configuration management, interfacing
 * with the job scheduler and expiring of all items produced by this
 * importer.
 *
 * When a FeedsImporter is instantiated, it loads its configuration. Then it
 * instantiates one fetcher, one parser and one processor plugin depending on
 * the configuration information. After instantiating them, it sets them to
 * the configuration information it holds for them.
 */
class FeedsImporter extends FeedsConfigurable {

  // Every feed has a fetcher, a parser and a processor.
  // These variable names match the possible return values of
  // FeedsPlugin::typeOf().
  protected $fetcher, $parser, $processor;

  // This array defines the variable names of the plugins above.
  protected $plugin_types = array('fetcher', 'parser', 'processor');

  /**
   * Instantiate class variables, initialize and configure
   * plugins.
   */
  protected function __construct($id) {
    parent::__construct($id);

    // Try to load information from database.
    $this->load();

    // Instantiate fetcher, parser and processor, set their configuration if
    // stored info is available.
    foreach ($this->plugin_types as $type) {
      $plugin = feeds_plugin($this->config[$type]['plugin_key'], $this->id);

      if (isset($this->config[$type]['config'])) {
        $plugin->setConfig($this->config[$type]['config']);
      }
      $this->$type = $plugin;
    }
  }

  /**
   * Remove items older than $time.
   *
   * @param $time
   *   All items older than REQUEST_TIME - $time will be deleted. If not
   *   given, internal processor settings will be used.
   *
   * @return
   *   FEEDS_BATCH_COMPLETE if the expiry process finished. A decimal between
   *   0.0 and 0.9 periodic if expiry is still in progress.
   *
   * @throws
   *   Throws Exception if an error occurs when expiring items.
   */
  public function expire($time = NULL) {
    return $this->processor->expire($time);
  }

  /**
   * Schedule this importer.
   */
  public function schedule() {
    $job = array(
      'type' => $this->id,
      'period' => 0,
      'periodic' => TRUE,
    );
    if (FEEDS_EXPIRE_NEVER != $this->processor->expiryTime()) {
      $job['period'] = 3600;
      JobScheduler::get('feeds_importer_expire')->set($job);
    }
    else {
      JobScheduler::get('feeds_importer_expire')->remove($job);
    }
  }

  /**
   * Report how many items *should* be created on one page load by this
   * importer.
   *
   * Note:
   *
   * It depends on whether parser implements batching if this limit is actually
   * respected. Further, if no limit is reported it doesn't mean that the
   * number of items that can be created on one page load is actually without
   * limit.
   *
   * @return
   *   A positive number defining the number of items that can be created on
   *   one page load. 0 if this number is unlimited.
   */
  public function getLimit() {
    return $this->processor->getLimit();
  }

  /**
   * Save configuration.
   */
  public function save() {
    $save = new stdClass();
    $save->id = $this->id;
    $save->config = $this->getConfig();
    if (db_query_range("SELECT 1 FROM {feeds_importer} WHERE id = :id", 0, 1, array(':id' => $this->id))->fetchField()) {
      drupal_write_record('feeds_importer', $save, 'id');
    }
    else {
      drupal_write_record('feeds_importer', $save);
    }
    // Clear menu cache, changes to importer can change menu items.
    menu_rebuild();
  }

  /**
   * Load configuration and unpack.
   */
  public function load() {
    ctools_include('export');
    if ($config = ctools_export_load_object('feeds_importer', 'conditions', array('id' => $this->id))) {
      $config = array_shift($config);
      $this->export_type = $config->export_type;
      $this->disabled = isset($config->disabled) ? $config->disabled : FALSE;
      $this->config = $config->config;
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Delete configuration. Removes configuration information
   * from database, does not delete configuration itself.
   */
  public function delete() {
    db_delete('feeds_importer')
      ->condition('id', $this->id)
      ->execute();
    $job = array(
      'type' => $this->id,
      'id' => 0,
    );
    if ($this->export_type & EXPORT_IN_CODE) {
      feeds_reschedule($this->id);
    }
    else {
      JobScheduler::get('feeds_importer_expire')->remove($job);
    }
  }

  /**
   * Set plugin.
   *
   * @param $plugin_key
   *   A fetcher, parser or processor plugin.
   *
   * @todo Error handling, handle setting to the same plugin.
   */
  public function setPlugin($plugin_key) {
    // $plugin_type can be either 'fetcher', 'parser' or 'processor'
    if ($plugin_type = FeedsPlugin::typeOf($plugin_key)) {
      if ($plugin = feeds_plugin($plugin_key, $this->id)) {
        // Unset existing plugin, switch to new plugin.
        unset($this->$plugin_type);
        $this->$plugin_type = $plugin;
        // Set configuration information, blow away any previous information on
        // this spot.
        $this->config[$plugin_type] = array('plugin_key' => $plugin_key);
      }
    }
  }

  /**
   * Copy a FeedsImporter configuration into this importer.
   *
   * @param FeedsImporter $importer
   *   The feeds importer object to copy from.
   */
  public function copy(FeedsImporter $importer) {
    $this->setConfig($importer->config);

    // Instantiate new fetcher, parser and processor and initialize their
    // configurations.
    foreach ($this->plugin_types as $plugin_type) {
      $this->setPlugin($importer->config[$plugin_type]['plugin_key']);
      $this->$plugin_type->setConfig($importer->config[$plugin_type]['config']);
    }
  }

  /**
   * Get configuration of this feed.
   */
  public function getConfig() {
    foreach (array('fetcher', 'parser', 'processor') as $type) {
      $this->config[$type]['config'] = $this->$type->getConfig();
    }
    return $this->config;// Collect information from plugins.
  }

  /**
   * Return defaults for feed configuration.
   */
  public function configDefaults() {
    return array(
      'name' => '',
      'description' => '',
      'fetcher' => array(
        'plugin_key' => 'FeedsHTTPFetcher',
      ),
      'parser' => array(
        'plugin_key' => 'FeedsSyndicationParser',
      ),
      'processor' => array(
        'plugin_key' => 'FeedsNodeProcessor',
      ),
      'content_type' => '',
      'update' => 0,
      'import_period' => 1800, // Refresh every 30 minutes by default.
      'expire_period' => 3600, // Expire every hour by default, this is a hidden setting.
      'import_on_create' => TRUE, // Import on submission.
    );
  }

  /**
   * Override parent::configForm().
   */
  public function configForm(&$form_state) {
    $form = array();
    $form['name'] = array(
      '#type' => 'textfield',
      '#title' => t('Name'),
      '#description' => t('The name of this configuration.'),
      '#default_value' => $this->config['name'],
      '#required' => TRUE,
    );
    $form['description'] = array(
      '#type' => 'textfield',
      '#title' => t('Description'),
      '#description' => t('A description of this configuration.'),
      '#default_value' => $this->config['description'],
    );
    $node_types = node_type_get_names();
    array_walk($node_types, 'check_plain');
    $form['content_type'] = array(
      '#type' => 'select',
      '#title' => t('Attach to content type'),
      '#description' => t('If an importer is attached to a content type, content is imported by creating a node. If the standalone form is selected, content is imported by using the standalone form under http://example.com/import.'),
      '#options' => array('' => t('Use standalone form')) + $node_types,
      '#default_value' => $this->config['content_type'],
    );
    $period = drupal_map_assoc(array(0, 900, 1800, 3600, 10800, 21600, 43200, 86400, 259200, 604800, 2419200), 'format_interval');
    $period[FEEDS_SCHEDULE_NEVER] = t('Never');
    $period[0] = t('As often as possible');
    $form['import_period'] = array(
      '#type' => 'select',
      '#title' => t('Minimum refresh period'),
      '#options' => $period,
      '#description' => t('This is the minimum time that must elapse before a feed may be refreshed automatically.'),
      '#default_value' => $this->config['import_period'],
    );
    $form['import_on_create'] = array(
      '#type' => 'checkbox',
      '#title' => t('Import on submission'),
      '#description' => t('Check if content should be imported at the moment of feed submission.'),
      '#default_value' => $this->config['import_on_create'],
    );
    return $form;
  }

  /**
   * Reschedule if import period changes.
   */
  public function configFormSubmit(&$values) {
    if ($this->config['import_period'] != $values['import_period']) {
      feeds_reschedule($this->id);
    }
    parent::configFormSubmit($values);
  }
}

/**
 * Helper, see FeedsDataProcessor class.
 */
function feeds_format_expire($timestamp) {
  if ($timestamp == FEEDS_EXPIRE_NEVER) {
    return t('Never');
  }
  return t('after !time', array('!time' => format_interval($timestamp)));
}
