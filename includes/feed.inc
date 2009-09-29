<?php
// $Id: feed.inc,v 1.17 2009/09/21 14:55:48 alexb Exp $
/**
 * @file
 */

/**
 * Importer class.
 */
class Feed extends FeedsConfigurable {

  public $fetcher, $parser, $processors; 

  /**
   * Instantiate class variables.
   */
  public function __construct($id, $config = NULL) {
    parent::__construct($id, $config);

    feeds_require_plugin($this->config['fetcher']);
    $this->fetcher = new $this->config['fetcher']($id);

    feeds_require_plugin($this->config['parser']);
    $this->parser = new $this->config['parser']($id);

    foreach ($this->config['processors'] as $processor) {
      feeds_require_plugin($processor);
      $this->processors[$processor] = new $processor($id);
    }
  }

  /**
   * Return defaults for feed configuration.
   */
  public function getDefaultConfig() {
    return array(
      'fetcher' => 'FeedsHttpFetcher',
      'parser' => 'FeedsSyndicationParser',
      'processors' => array('FeedsNodeProcessor'),
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
   * Feed class specific plugin form.
   */
  public function pluginForm(&$form_state) {
    $form = array();
    $plugins = feeds_get_plugins();
    $form['fetcher'] = array(
      '#type' => 'radios',
      '#title' => t('Fetcher'),
      '#options' => $plugins['fetcher'],
      '#description' => t('Select a fetcher for this configuration.'),
      '#default_value' => $this->config['fetcher'],
    );
    $form['parser'] = array(
      '#type' => 'radios',
      '#title' => t('Parser'),
      '#options' => $plugins['parser'],
      '#description' => t('Select a parser for this configuration.'),
      '#default_value' => $this->config['parser'],
    );
    $form['processors'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Processors'),
      '#options' => $plugins['processor'],
      '#description' => t('Select processors for this configuration.'),
      '#default_value' => $this->config['processors'],
    );
    return $form;
  }

  /**
   * Validation handler for pluginForm().
   */
  public function pluginFormValidate($form, &$form_state) {
    $form_state['values']['processors'] = array_filter($form_state['values']['processors']);
    if (!count($form_state['values']['processors'])) {
      form_set_error('processors', t('At least one processor needs to be enabled.'));
    }
  }

  /**
   * Submit handler for pluginForm().
   */
  public function pluginFormSubmit($form, &$form_state) {
    $this->addConfig($form_state['values']);
    $this->save();
  }

  /**
   * Import feed by using configured fetchers, parsers, processors.
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
   *   String that is the class name of the fetcher.
   */
  public function setFetcher($fetcher) {
    unset($this->fetcher);
    feeds_require_plugin($fetcher);
    $this->config['fetcher'] = $fetcher;
    $this->fetcher = new $fetcher($this->id);
  }

  /**
   * Set active parser. Does not save parser to configuration.
   *
   * @param $parser
   *   String that is the class name of the parser.
   */
  public function setParser($parser) {
    unset($this->parser);
    feeds_require_plugin($parser);
    $this->config['parser'] = $parser;
    $this->save();
    $this->parser = new $parser($this->id);
  }

  /**
   * Set active processors.
   *
   * @param $processors
   *  Array of strings that are the processors of this feed.
   */
  public function setProcessors($processors) {
    unset($this->processors);
    $this->processors = array();
    $this->config['processors'] = $processors;
    $this->save();
    foreach ($processors as $processor) {
      feeds_require_plugin($processor);
      $this->processors[] = new $processor($this->id);
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
   * Constructor.
   *
   * @param $id
   *   String identifier of this object.
   * @param $config
   *   Configuration of this object. If not available, will attempt to load from database.
   */
  public function __construct($id, $config = NULL) {
    $this->id = $id;
    if (empty($config)) {
      if (!$this->load()) {
        // Make sure configuration is populated.
        $this->config = $this->getDefaultConfig();
      }
    }
    else {
      $this->config = $config;
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
   * Retrieve this configurable's Id.
   * 
   * By convention, a number of configurable objects are associated to each 
   * other by their common id.
   */
  public function getId() {
    return $this->id;
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
   * Get configuration.
   * 
   * @todo: clean up fallback. Not clear when to use and when not to use getConfig().
   * 
   * @param $fallback
   *   Set to false if method should NOT fall back to default configuration.
   */
  public function getConfig($fallback = TRUE) {
    if ($fallback) {
      return empty($this->config) ? $this->getDefaultConfig() : $this->config;
    }
    return $this->config;
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
    $this->save();
  }
}

/**
 * Abstract class, defines interface for fetchers.
 * 
 * Not using interfaces because we need a simple inheritence tree for determining the 
 * plugin type. See hook_feeds_plugin().
 */
class FeedsFetcher extends FeedsConfigurable {

  /**
   * Source form.
   */
  public function sourceForm(&$form_state) {
    $form = array();
    $form['source'] = array(
      '#type' => 'textfield',
      '#title' => t('URL'),
      '#description' => t('Enter the URL for this feed.'),
      '#default_value' => $this->config['source'],
    );
    return $form;
  }

  /**
   * Fetch content from a source and return it.
   * 
   * Stub method. Every class that extends FeedsFetcher must implement this method.
   *
   * @param $source 
   *   Source value as entered by user through sourceForm().
   * 
   * @todo: Define format of return value?
   */
  public function fetch($source) {
    return NULL;
  }
}

/**
 * Abstract class, defines interface for parsers.
 */
class FeedsParser extends FeedsConfigurable {

  /**
   * Parse content fetched by fetcher.
   * 
   * Stub method. Extending classes must implement this method.
   *
   * @param $raw
   *   Content returned by fetcher.
   * @return
   *   A parsed array. 
   *   @todo: define this array (object?).
   */
  public function parse($raw) {
    return NULL;
  }

  /**
   * Declare the possible mapping sources that this parser produces.
   * 
   * @return 
   *   An array of mapping sources.
   */
  public function getMappingSources() {
    return NULL;
  }
}

/**
 * Abstract class, defines interface for processors.
 */
class FeedsProcessor extends FeedsConfigurable {

  /**
   * Process the result of the parser or previous processors.
   * 
   * Stub method. Extending classes must implement this method.
   *
   * @param $feed
   *   Result returned by parser.
   */
  public function process($feed) {
  }

  /**
   * Declare default configuration.
   */
  public function getDefaultConfig() {
    return array('mappings' => FALSE);
  }

  /**
   * Add a mapping to existing mappings.
   */
  public function addMapping($source, $target, $unique = NULL) {
    if (!empty($source) && !empty($target)) {
      $this->config['mappings'][$target] = array(
        'source' => $source,
        'unique' => $unique,
      );
    }
    $this->save();
  }

  /**
   * Remove a mapping.
   */
  public function removeMapping($target) {
    unset($this->config['mappings'][$target]);
    $this->save();
  }

  /**
   * Declare possible mapping targets.
   *
   * @return
   *   An array of mapping targets. Keys are paths to targets
   *   separated by ->, values are TRUE if target can be unique, 
   *   FALSE otherwise.
   */
  public function getMappingTargets() {
    // @todo: invoke hook_feeds_mapper() here.
    return array();
  }

  /**
   * Get mappings.
   */
  public function getMappings() {
    return $this->config['mappings'];
  }

  /**
   * Execute mapping on an item.
   */
  public function map($item) {
    // @todo.
  }

  /**
   * Determine whether a given item is unique.
   */
  public function unique($item) {
    // @todo.
  }
}