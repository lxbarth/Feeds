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
 * Configurable importer class. This is the main hub for Feeds module's functionality.
 */
class Feed extends FeedsConfigurable {

  // Every feed has a fetcher, a parser and a processor.
  // These variable names match the possible return values of feeds_plugin_type().
  protected $fetcher, $parser, $processor;

  /**
   * Instantiate class variables.
   */
  protected function __construct($id) {
    parent::__construct($id);

    $this->fetcher = feeds_plugin_instance($this->config['fetcher'], $id);
    $this->parser = feeds_plugin_instance($this->config['parser'], $id);
    $this->processor = feeds_plugin_instance($this->config['processor'], $id);
  }

  /**
   * Return defaults for feed configuration.
   */
  public function getDefaultConfig() {
    return array(
      'name' => '',
      'fetcher' => 'FeedsHttpFetcher',
      'parser' => 'FeedsSyndicationParser',
      'processor' => 'FeedsNodeProcessor',
      'content_type' => '',
      'update' => 0,
      'refresh_period' => FEEDAPI_CRON_NEVER_REFRESH,
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
    $period[FEEDAPI_CRON_NEVER_REFRESH] = t('Do not refresh periodically');
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
   * @todo: error handling.
   */
  public function setPlugin($plugin_key) {
    // $plugin_type can be either 'fetcher', 'parser' or 'processor'
    if ($plugin_type = feeds_plugin_type($plugin_key)) {
      if ($class = feeds_load_plugin_class($plugin_key)) {
        // Remove existing pointer to object.
        unset($this->$plugin_type);
        // Set configuration information.
        $this->config[$plugin_type] = $plugin_key;
        // Update pointer to object.
        $this->$plugin_type = feeds_instance($class, $this->id);
      }
    }
  }
}

/**
 * Base class for configurable, peristent objects.
 *
 * Note: setters don't save results. A configurable can be used without actually creating
 * a record in the database. Only when save() is being called, a configurable is being
 * written to the database. Thus configurables can be used as temporary objects.
 */
class FeedsConfigurable {
  protected $config;
  protected $id;

  /**
   * Instantiate a FeedsConfigurable object.
   *
   * Don't use directly, use feeds_instance() instead.
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
   * Constructor.
   *
   * @param $id
   *   String identifier of this object.
   * @param $config
   *   Configuration of this object. If not available, will attempt to load from database.
   */
  protected function __construct($id) {
    $this->id = $id;
    if (!$this->load()) {
      // Make sure configuration is populated.
      $this->config = $this->getDefaultConfig();
    }
  }

  /**
   * Save configuration.
   */
  public function save() {
    $save = new stdClass();
    $save->id = $this->id;
    $save->class = get_class($this);
    $save->config = $this->config;
    db_query('DELETE FROM {feeds_config} WHERE id = "%s" AND class = "%s"', $save->id, $save->class);
    drupal_write_record('feeds_config', $save);
  }

  /**
   * Load configuration and unpack.
   */
  public function load() {
    ctools_include('export');
    if ($config = ctools_export_load_object('feeds_config', 'conditions', array('id' => $this->id, 'class' => get_class($this)))) {
      $config = array_shift($config);
      $this->config = $config->config;
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Return this configurable's protected variables.
   *
   * Called automatically when accessing protected variables from outside.
   * e. g. print $object->id;
   *
   * @see http://us2.php.net/__get
   */
  public function __get($name) {
    return $this->$name;
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
   * @todo: save() automatically?
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
    // Save here. Manipulation through form assumes that the FeedsConfigurable
    // object is persistent (DB or code).
    $this->save();
  }
}

/**
 * Config form wrapper. Use to render the configuration form of
 * a FeedsConfigurable object.
 *
 * @param
 *   FeedsConfigurable object.
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
}
