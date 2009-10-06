<?php
// $Id: feed.inc,v 1.17 2009/09/21 14:55:48 alexb Exp $

/**
 * @file
 *
 * Home of Feed Class and related interfaces.
 */

// Including Feed.inc automatically includes dependencies.
require_once(dirname(__FILE__) .'/FeedsConfigurable.inc');
require_once(dirname(__FILE__) .'/FeedsSource.inc');

/**
 * A Feeds result class.
 *
 * @see class FeedsFetcherResult
 * @see class FeedsParserResult
 */
abstract class FeedsResult {

  protected $type;
  protected $value;
  // An array of valid values for $type.
  protected $validTypes = array();

  /**
   * Constructor: create object, validate class variables.
   *
   * @param $value
   *   The value of this result.
   * @param $type
   *   The type of this result. Must be one of $validTypes.
   */
  public function __construct($value, $type) {
    $this->_set('type', $type);
    $this->_set('value', $value);
  }

  /**
   * Control access to class variables.
   */
  public function __get($name) {
    return $this->$name;
  }

  /**
   * Control access to class variables.
   */
  public function _set($name, $value) {
    if ($name == 'validTypes') {
      throw new Exception('Cannot write FeedsResult::validTypes.');
    }
    if ($name == 'type') {
      if (!in_array($value, $this->validTypes)) {
        throw new Exception('Invalid type "'. $value .'"');
      }
    }
    $this->$name = $value;
  }
}

/**
 * Configurable importer class. This is the main hub for Feeds module's functionality.
 *
 * @todo: rename to FeedsFeedConfig.
 */
class Feed extends FeedsConfigurable {

  // Every feed has a fetcher, a parser and a processor.
  // These variable names match the possible return values of feeds_plugin_type().
  protected $fetcher, $parser, $processor;

  // This array defines the variable names of the plugins above.
  // @todo: make this static once 5.3.0 has spread enough and __callstatic can be used.
  // @todo use.
  protected $types = array('fetcher', 'parser', 'processor');

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
    foreach (array('fetcher', 'parser', 'processor') as $type) {
      $plugin = feeds_plugin_instance($this->config[$type]['plugin_key'], $this->id);
      if (isset($this->config[$type]['config'])) {
        $plugin->setConfig($this->config[$type]['config']);
      }
      $this->$type = $plugin;
    }
  }

  /**
   * Save configuration.
   */
  public function save() {
    $save = new stdClass();
    $save->id = $this->id;
    $save->class = get_class($this);
    $save->config = $this->getConfig();
    // @todo: don't delete, then insert.
    $this->delete();
    drupal_write_record('feeds_config', $save);
  }

  /**
   * Load configuration and unpack.
   */
  public function load() {
    ctools_include('export');
    if ($config = ctools_export_load_object('feeds_config', 'conditions', array('id' => $this->id, 'class' => get_class($this)))) {
      $config = array_shift($config);
      $this->export_type = $config->export_type;
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
    db_query('DELETE FROM {feeds_config} WHERE id = "%s" AND class = "%s"', $this->id, get_class($this));
  }

  /**
   * Return defaults for feed configuration.
   */
  public function getDefaultConfig() {
    return array(
      'name' => '',
      'fetcher' => array(
        'plugin_key' => 'FeedsHttpFetcher',
      ),
      'parser' => array(
        'plugin_key' => 'FeedsSyndicationParser',
      ),
      'processor' => array(
        'plugin_key' => 'FeedsNodeProcessor',
      ),
      'content_type' => '',
      'update' => 0,
      'refresh_period' => FEEDS_SCHEDULE_NEVER_REFRESH,
    );
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
    $form['content_type'] = array(
      '#type' => 'select',
      '#title' => t('Attach to content type'),
      '#description' => t('If you attach a configuration to a node you can use nodes for creating feeds on your site.'),
      '#options' => array('' => t('None')) + node_get_types('names'),
      '#default_value' => $this->config['content_type'],
    );
    $period = drupal_map_assoc(array(1, 900, 1800, 3600, 10800, 21600, 43200, 86400, 259200, 604800, 2419200), 'format_interval');
    $period[FEEDS_SCHEDULE_NEVER_REFRESH] = t('Never');
    $period[1] = t('As often as possible');
    $form['refresh_period'] = array(
      '#type' => 'select',
      '#title' => t('Minimum refresh period'),
      '#options' => $period,
      '#description' => t('This is the minimum time that must elapse before a feed may be refreshed automatically.'),
      '#default_value' => $this->config['refresh_period'],
    );
    return $form;
  }

  /**
   * Override parent::configFormSubmit().
   */
  public function configFormSubmit($form, &$form_state) {
    parent::configFormSubmit($form, $form_state);
    // Clear cache, changes to content_type have an affect on menu tree.
    menu_rebuild();
  }

  /**
   * Set plugin.
   *
   * @param $plugin_key
   *   A fetcher, parser or processor plugin.
   *
   * @todo: error handling, handle setting to the same plugin.
   */
  public function setPlugin($plugin_key) {
    // $plugin_type can be either 'fetcher', 'parser' or 'processor'
    if ($plugin_type = feeds_plugin_type($plugin_key)) {
      if ($plugin = feeds_plugin_instance($plugin_key, $this->id)) {
        // Unset existing plugin, switch to new plugin.
        unset($this->$plugin_type);
        $this->$plugin_type = $plugin;
        // Set configuration information, blow away any previous information on this spot.
        $this->config[$plugin_type] = array('plugin_key' => $plugin_key);
      }
    }
  }
}