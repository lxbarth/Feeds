<?php
// $Id$

/**
 * @file
 * Home of FeedsConfigurable class and related.
 */

/**
 * Define a configurable interface.
 *
 * Note how a configurable does not need to implement any DB persistence
 * methods like load(), save() or delete().
 *
 * @todo: comment methods.
 */
interface FeedsConfigurableInterface {
  // @todo: don't pass trough $form_state.
  public function configForm(&$form_state);
  public function getConfig();
  public function setConfig($config);
  public function addConfig($config);
  // @todo: rename to getConfigDefaults()
  public function getDefaultConfig();
}

/**
 * Base class for configurable objects.
 *
 * Contains some common variables and functionality for implementers
 * of FeedsConfigurableInterface and a static method to generate singletons.
 */
class FeedsConfigurable implements FeedsConfigurableInterface {

  // Holds the actual configuration information.
  protected $config;

  // A unique identifier for the configuration.
  protected $id;

  // Export type can be one of
  // FEEDS_EXPORT_NONE - the configurable only exists in memory
  // EXPORT_IN_DATABASE - the configurable is defined in the database.
  // EXPORT_IN_CODE - the configurable is defined in code.
  // EXPORT_IN_CODE | EXPORT_IN_DATABASE -
  // - the configurable is defined in code, but overridden in the database.
  protected $export_type;

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
    // Set this object's id.
    $this->id = $id;

    // Per default we assume that a Feeds object is not saved to
    // database nor is it exported to code.
    $this->export_type = FEEDS_EXPORT_NONE;

    // Make sure configuration is populated.
    $this->config = $this->getDefaultConfig();
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
   * Implementation of getConfig().
   */
  public function getConfig() {
    return $this->config;
  }

  /**
   * Return default configuration.
   *
   * @todo: rename to getConfigDefaults().
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
