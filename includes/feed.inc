<?php
// $Id: feed.inc,v 1.17 2009/09/21 14:55:48 alexb Exp $
/**
 * @file
 */

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
 * Define a configurable interface.
 *
 * Note how a configurable does not need to implement any DB persistence
 * methods like load(), save() or delete().
 *
 * @todo: comment methods.
 */
interface FeedsConfigurableInterface {
  public function configForm(&$form_state);
  public function getConfig();
  public function setConfig($config);
  public function addConfig($config);
  public function getDefaultConfig();
}

/**
 * Base class for configurable objects.
 *
 * Contains some common variables and functionality for implementers
 * of FeedsConfigurableInterface and a static method to generate singletons.
 */
class FeedsConfigurable implements FeedsConfigurableInterface {
  protected $config;
  protected $id;

  /**
   * Instantiate a FeedsConfigurable object.
   *
   * Don't use directly, use feeds_feed_instance() or feeds_plugin_instance()
   * instead.
   */
  public static function instance($class, $id) {
    // This is useful at least as long as we're developing.
    if (empty($id)) {
      throw new Exception('Empty configuration identifier.');
    }
    static $instances = array();
    if (!isset($instances[$class][$id])) {
      $instances[$class][$id] = new $class($id);
    }
    return $instances[$class][$id];
  }

  /**
   * Constructor, set id and load default configuration.
   */
  protected function __construct($id) {
    $this->id = $id;
    // Make sure configuration is populated.
    $this->config = $this->getDefaultConfig();
  }

  /**
   * Implementation of getConfig().
   */
  public function getConfig() {
    return $this->config;
  }

  /**
   * Return default configuration.
   *
   * @return
   *   Array where keys are the variable names of the configuration elements and
   *   values are their default values.
   */
  public function getDefaultConfig() {
    $fake_form_state = array();
    $form = $this->configForm($fake_form_state);
    return element_children($form);
  }

  /**
   * Set configuration.
   *
   * @param $config
   *   Array containing configuration information. Will be filtered by the keys returned by
   *   getDefaultConfig().
   */
  public function setConfig($config) {
    $default_keys = $this->getDefaultConfig();
    $this->config = array_intersect_key($config, $default_keys);
  }

  /**
   * Similar to setConfig but adds to existing configuration.
   *
   * @param $config
   *   Array containing configuration information. Will be filtered by the keys returned by
   *   getDefaultConfig().
   */
  public function addConfig($config) {
    $this->config = array_merge($this->config, $config);
    $default_keys = $this->getDefaultConfig();
    $this->config = array_intersect_key($this->config, $default_keys);
  }

  /**
   * Return configuration form for this object.
   *
   * @return FormAPI style form definition.
   */
  public function configForm(&$form_state) {
    return array();
  }

  /**
   * Validation handler for configForm().
   */
  public function configFormValidate($form, &$form_state) {
  }

  /**
   * Submit handler for configForm().
   */
  public function configFormSubmit($form, &$form_state) {
    $this->addConfig($form_state['values']);
  }
}

/**
 * Configurable importer class. This is the main hub for Feeds module's functionality.
 *
 * @todo: rename to FeedsConfig. "Feed" remains a very elusive concept.
 */
class Feed extends FeedsConfigurable {

  // Every feed has a fetcher, a parser and a processor.
  // These variable names match the possible return values of feeds_plugin_type().
  protected $fetcher, $parser, $processor;

  // Export type can be one of
  protected $export_type;

  /**
   * Instantiate class variables, initialize and configure
   * plugins.
   */
  protected function __construct($id) {
    parent::__construct($id);

    // Per default we assume that a Feeds object is not saved to
    // database nor is it exported to code.
    $this->export_type = FEEDS_EXPORT_NONE;

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
   * Override magic method __get(). Make sure that $this->config goes
   * through getConfig()
   */
  public function __get($name) {
    if ($name == 'config') {
      return $this->getConfig();
    }
    return $this->$name;
  }

  /**
   * Get configuration of this feed.
   */
  public function getConfig() {
    // Collect information from plugins.
    foreach (array('fetcher', 'parser', 'processor') as $type) {
      $this->config[$type]['config'] = $this->$type->getConfig();
    }
    return $this->config;
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
   * Import feed by using configured fetchers, parsers, processor.
   *
   * @param $feed_nid
   *   If this configuaration is attached to a feed node, $nid contains the
   *   node id of the feed node.
   *
   * @todo: Break out into a facade-style FeedsImport class?
   * @todo: Pass in source object, not ominous identifier $feed_nid.
   *        Figure out how to reconcile source object with nid handling.
   *        We may need to think of per-import (= per-node) configurations in a larger context.
   *        For instance, right now a parser or a processor can't offer any one-off configuration options
   *        for imports. Think of delimiter settings for CSV imports.
   *
   * @throws Exception
   *   If a problem with fetching, parsing or processing occured.
   */
  public function import($feed_nid = NULL) {
    $result = $this->fetcher->fetch($feed_nid);
    $result = $this->parser->parse($result, $feed_nid);
    $this->processor->process($result, $feed_nid);
  }

  /**
   * Remove all imported items.
   */
  public function purge($feed_nid = NULL) {
    $this->processor->purge($feed_nid);
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

/**
 * Config form wrapper. Use to render the configuration form of
 * a FeedsConfigurable object.
 *
 * @param
 *   FeedsConfigurable object.
 * @param
 *   The parent object to perform the save on.
 *
 * @return
 *   Rendered config form, if available. Empty string otherwise.
 */
function feeds_get_config_form($configurable) {
  if ($configurable->configForm($form_state)) {
    return drupal_get_form(get_class($configurable) .'_feeds_config_form', $configurable);
  }
  return '';
}

/**
 * Config form callback. Don't call directly, but use
 * feeds_get_config_form($configurable) instead.
 *
 * @param
 *   FormAPI $form_state.
 * @param
 *   FeedsConfigurable object.
 * @param
 *   The object to perform the save() operation on.
 */
function feeds_config_form(&$form_state, $configurable) {
  $form = $configurable->configForm($form_state);
  $form['#configurable'] = $configurable;
  $form['#validate'] = array('feeds_config_form_validate');
  $form['#submit'] = array('feeds_config_form_submit');
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  return $form;
}

/**
 * Validation handler for feeds_config_form().
 */
function feeds_config_form_validate($form, &$form_state) {
  $form['#configurable']->configFormValidate($form, $form_state);
}

/**
 * Submit handler for feeds_config_form().
 */
function feeds_config_form_submit($form, &$form_state) {
  $form['#configurable']->configFormSubmit($form, $form_state);
  $form['#configurable']->save();
}
