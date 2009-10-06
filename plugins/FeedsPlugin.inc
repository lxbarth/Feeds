<?php
// $Id$

/**
 * @file
 * Definition of FeedsPlugin class.
 */

/**
 * Implement source interface for all plugins.
 */
abstract class FeedsPlugin extends FeedsConfigurable implements FeedsSourceInterface {

  // Holds source information, extending classes should always
  // go through setters and getters.
  private $source_config;

  /**
   * Constructor.
   *
   * Initialize class variables.
   */
  protected function __construct($id) {
    parent::__construct($id);
    $this->source_config = $this->getSourceDefaults();
  }

  /**
   * For convenience, expose class variables through __get().
   */
  public function __get($name) {
    return $this->$name;
  }

  /**
   * Returns TRUE if $this->sourceForm() returns a form.
   */
  public function hasSourceConfig() {
    $form = $this->sourceForm();
    return !empty($form);
  }

  /**
   * Implementation of FeedsSourceInterface::getSourceConfig().
   *
   * This method is being used by FeedsSource to collect source information
   * from this plugin.
   */
  public function getSourceConfig() {
    return $this->source_config;
  }

  /**
   * Implementation of FeedsSourceInterface::setSourceConfig().
   */
  public function setSourceConfig($config) {
    $this->source_config = $config;
  }

  /**
   * Implementation of FeedsSourceInterface::getSourceDefaults().
   */
  public function getSourceDefaults() {
    $values = array_flip(array_keys($this->sourceForm()));
    foreach ($values as $k => $v) {
      $values[$k] = '';
    }
    return $values;
  }

  /**
   * Callback methods, exposes source form.
   */
  public function sourceForm() {
    return array();
  }

  /**
   * Validation handler for sourceForm.
   */
  public function sourceFormValidate($form, &$values) {}

  /**
   * Submit handler for sourceForm.
   */
  public function sourceFormSubmit($form, &$values) {
    $this->setSourceConfig($values);
  }

}