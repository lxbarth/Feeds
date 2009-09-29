<?php
// $Id: FeedsCSVParser.inc,v 1.1 2009/09/16 22:08:48 alexb Exp $

class FeedsCSVParser extends FeedsParser {

  /**
   * Parses a raw string and returns a Feed object from it.
   */
  public function parse($raw) {
    
  }

  /**
   * Return mapping sources.
   */
  public function getMappingSources() {
    return array();
  }

  public function getDefaultConfig() {
    return array('delimiter' => ',');
  }

  public function configForm(&$form_state) {
    $form = array();
    $form['delimiter'] = array(
      '#type' => 'textfield',
      '#title' => t('Delimiter'),
      '#description' => t('Field delimeter. E. g. , or ;'),
      '#default_value' => $this->config['delimiter'],
    );
    return $form;
  }
}