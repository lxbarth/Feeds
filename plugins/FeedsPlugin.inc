<?php
// $Id$

/**
 * @file
 * Definition of FeedsPlugin class.
 */

/**
 * Implement source interface for all plugins.
 *
 * Note how this class does not attempt to store source information locally.
 * Doing this would break the model where a Source information is strictly an
 * object that is being passed through Feed and its plugins.
 *
 * There is only one instance of a Feed or a FeedsPlugin per feed configuration,
 * whereas there is one instance of a FeedsSource per feed configuration, per
 * feed info definition (i. e. per feed configuaration, per feed node + standalone
 * import form).
 */
abstract class FeedsPlugin extends FeedsConfigurable implements FeedsSourceInterface {

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
   * Save changes to the configuration of this object.
   * Delegate saving to parent (= Feed) which will collect
   * information from this object by way of getConfig() and store it.
   */
  public function save() {
    $feed = feeds_feed($this->id)->save();
  }

  /**
   * Returns TRUE if $this->sourceForm() returns a form.
   */
  public function hasSourceConfig() {
    $form = $this->sourceForm(array());
    return !empty($form);
  }

  /**
   * Implementation of FeedsSourceInterface::getSourceDefaults().
   */
  public function getSourceDefaults() {
    $values = array_flip(array_keys($this->sourceForm(array())));
    foreach ($values as $k => $v) {
      $values[$k] = '';
    }
    return $values;
  }

  /**
   * Callback methods, exposes source form.
   */
  public function sourceForm($source_config) {
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