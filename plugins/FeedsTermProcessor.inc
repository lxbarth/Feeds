<?php
// $Id$

/**
 * @file
 * Create a taxonomy terms from feed items.
 */
class FeedsTermProcessor extends FeedsProcessor {

  /**
   * Implementation of FeedsProcessor::process().
   */
  public function process(FeedsParserResult $parserResult, FeedsSource $source) {

  }

  /**
   * Implement purge.
   */
  public function purge(FeedsSource $source) {

  }

  /**
   * Execute mapping on an item.
   */
  protected function map($source_item) {
    // Prepare term object.
    $target_term = array();
    $target_term['vid'] = $this->config['vocabulary'];

    // Have parent class do the iterating.
    return parent::map($source_item, $target_term);
  }

  /**
   * Override parent::configDefaults().
   */
  public function configDefaults() {
    return array(
      'vocabulary' => 0,
      'update_existing' => 0,
    );
  }

  /**
   * Override parent::configForm().
   */
  public function configForm(&$form_state) {
    $options = array(0 => t('Select a vocabulary'));
    foreach (taxonomy_get_vocabularies() as $vid => $vocab) {
      $options[$vid] = $vocab->name;
    }
    $form = array();
    $form['vocabulary'] = array(
      '#type' => 'select',
      '#title' => t('Vocabulary'),
      '#description' => t('Choose the vocabulary to import into.'),
      '#options' => $options,
      '#default_value' => $this->config['vocabulary'],
    );
    $form['update_existing'] = array(
      '#type' => 'checkbox',
      '#title' => t('Update existing items'),
      '#description' => t('Check if existing terms should be updated from the feed.'),
      '#default_value' => $this->config['update_existing'],
    );
    return $form;
  }

  /**
   * Return available mapping targets.
   */
  public function getMappingTargets() {
    $targets = array(
      'name' => array(
        'name' => t('Term name'),
        'optional_unique' => TRUE,
       ),
    );
    // @todo: document.
    drupal_alter('feeds_term_processor_targets', $targets);
    return $targets;
  }

  /**
   * Get nid of an existing feed item node if available.
   */
  protected function existingItemId($source_item, FeedsSource $source) {

    // The only possible unique target is name.
    foreach ($this->uniqueTargets($source_item) as $target => $value) {
      if ($target == 'name') {
        if ($tid = db_result(db_query('SELECT tid FROM {term_data} WHERE name = "%s" AND vid = %d', $value, $this->config['vocabulary']))) {
          return $tid;
        }
      }
    }
    return 0;
  }
}