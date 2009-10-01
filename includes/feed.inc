<?php
// $Id: feed.inc,v 1.17 2009/09/21 14:55:48 alexb Exp $
/**
 * @file
 */

/**
 * Importer class.
 */
class Feed extends FeedsConfigurable {

  protected $fetcher, $parser, $processors;

  /**
   * Instantiate class variables.
   */
  protected function __construct($id) {
    parent::__construct($id);

    $this->fetcher = feeds_plugin_instance($this->config['fetcher'], $id);

    $this->parser = feeds_plugin_instance($this->config['parser'], $id);

    foreach ($this->config['processors'] as $processor) {
      $this->processors[$processor] = feeds_plugin_instance($processor, $id);
    }
  }

  /**
   * Return defaults for feed configuration.
   */
  public function getDefaultConfig() {
    return array(
      'name' => '',
      'fetcher' => 'FeedsHttpFetcher',
      'parser' => 'FeedsSyndicationParser',
      'processors' => array('FeedsNodeProcessor' => 'FeedsNodeProcessor'),
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
    $form['source'] = $this->fetcher->sourceForm($form_state);
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
   * Import feed by using configured fetchers, parsers, processors.
   * 
   * @todo: Break out into a facade-style FeedsImport class?
   */
  public function import() {
    $raw = $this->fetcher->fetch($this->source);
    $parsed = $this->parser->parse($raw);
    foreach ($this->processors as $processor) {
      $processor->process($parsed);
    }
  }

  /**
   * Set active fetcher. Does not save fetcher to configuration.
   *
   * @param $fetcher
   *   String that is the plugin id of a fetcher.
   */
  public function setFetcher($fetcher) {
    if ($class = feeds_load_plugin_class($fetcher)) {
      unset($this->fetcher);
      $this->config['fetcher'] = $fetcher;
      $this->fetcher = new $class($this->id);
    }
  }

  /**
   * Set active parser. Does not save parser to configuration.
   *
   * @param $parser
   *   String that is the plugin id of a parser.
   */
  public function setParser($parser) {
    if ($class = feeds_load_plugin_class($parser)) {
      unset($this->parser);
      $this->config['parser'] = $parser;
      $this->parser = new $class($this->id);
    }
  }

  /**
   * Set active processors.
   *
   * @param $processors
   *  Array of strings that are plugin ids of processors.
   */
  public function setProcessors($processors) {
    foreach ($this->processors as $processor) {
      // @todo: verify that this actually distroys the objects.
      unset($processor);
    }
    $this->processors = array();
    $this->config['processors'] = drupal_map_assoc($processors);
    foreach ($processors as $processor) {
      $class = feeds_load_plugin_class($processor);
      $this->processors[$processor] = new $class($this->id);
    }
  }

  /**
   * Add a processor.
   * 
   * @param $processor
   *   String that is the plugin id of a processor.
   */
  public function addProcessor($processor) {
    if (!isset($this->processors[$processor])) {
      if ($class = feeds_load_plugin_class($processor)) {
        $this->processors[$processor] = new $class($this->id);
        $this->config['processors'][$processor] = $processor;
      }
    }
  }

  /**
   * Remove a processor.
   * 
   * @param $processor
   *   String that is the plugin id of a processor.
   */
  public function removeProcessor($processor) {
    if (isset($this->processors[$processor])) {
      unset($this->processors[$processor]);
      unset($this->config['processors'][$processor]);
    }
  }
}

/**
 * Base class for configurable, peristent objects.
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
    return array();
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
  $form['#configurable']->configFormValidate($form, &$form_state);
}

/**
 * Submit handler for feeds_config_form().
 */
function feeds_config_form_submit($form, &$form_state) {
  $form['#configurable']->configFormSubmit($form, &$form_state);
}
