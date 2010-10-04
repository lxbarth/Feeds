<?php
// $Id: FeedsTermProcessor.inc,v 1.25.2.2 2010/10/04 01:33:43 alexb Exp $

/**
 * @file
 * FeedsTermProcessor class.
 */

/**
 * Feeds processor plugin. Create taxonomy terms from feed items.
 */
class FeedsTermProcessor extends FeedsProcessor {
  /**
   * Define entity type.
   */
  protected function __construct($id) {
    parent::__construct($id);
    $this->entity_type = 'taxonomy_term';
  }


  /**
   * Implements FeedsProcessor::process().
   */
  public function process(FeedsSource $source, FeedsParserResult $parser_result) {

    if (empty($this->config['vocabulary'])) {
      throw new Exception(t('You must define a vocabulary for Taxonomy term processor before importing.'));
    }

    // Count number of created and updated nodes.
    $created  = $updated = $no_name = 0;

    while ($item = $parser_result->shiftItem()) {

      if (!($tid = $this->existingItemId($source, $parser_result)) || $this->config['update_existing'] != FEEDS_SKIP_EXISTING) {

        // Map item to a term.
        if ($tid && $this->config['update_existing'] == FEEDS_UPDATE_EXISTING) {
          $term = $this->loadTerm($source, $tid);
        }
        else {
          $term = $this->newTerm($source);
        }
        $term = $this->map($source, $parser_result, $term);

        // Check if term name is set, otherwise continue.
        if (empty($term->name)) {
          $no_name++;
          continue;
        }

        // Add term id if available.
        if (!empty($tid)) {
          $term->tid = $tid;
        }

        // Save the term.
        $term->feeds_importer_id = $this->id;
        $term->feed_nid = $source->feed_nid;
        taxonomy_term_save($term);
        if ($tid) {
          $updated++;
        }
        else {
          $created++;
        }
      }
    }

    // Set messages.
    $vocabulary = $this->vocabulary();
    if ($no_name) {
      drupal_set_message(
        format_plural(
          $no_name,
          'There was @number term that could not be imported because their name was empty. Check mapping settings on Taxomy term processor.',
          'There were @number terms that could not be imported because their name was empty. Check mapping settings on Taxomy term processor.',
          array('@number' => $no_name)
        ),
        'error'
      );
    }
    if ($created) {
      drupal_set_message(format_plural($created, 'Created @number term in !vocabulary.', 'Created @number terms in !vocabulary.', array('@number' => $created, '!vocabulary' => $vocabulary->name)));
    }
    elseif ($updated) {
      drupal_set_message(format_plural($updated, 'Updated @number term in !vocabulary.', 'Updated @number terms in !vocabulary.', array('@number' => $updated, '!vocabulary' => $vocabulary->name)));
    }
    else {
      drupal_set_message(t('There are no new terms.'));
    }
  }

  /**
   * Implements FeedsProcessor::clear().
   */
  public function clear(FeedsSource $source) {
    $deleted = 0;
    $vocabulary = $this->vocabulary();
    $terms = db_query("SELECT td.tid
                        FROM {taxonomy_term_data} td
                        JOIN {feeds_item} fi
                        ON fi.entity_type = 'taxonomy_term'
                        AND td.tid = fi.entity_id
                        WHERE td.vid = :vid
                        AND fi.id = :id
                        AND fi.feed_nid = :feed_nid",
                        array(
                          ':vid' => $vocabulary->vid,
                          ':id' => $this->id,
                          ':feed_nid' => $source->feed_nid,
                        ));
    foreach ($terms as $term) {
      if (taxonomy_term_delete($term->tid) == SAVED_DELETED) {
        $deleted++;
      }
    }
    // Set messages.
    if ($deleted) {
      drupal_set_message(format_plural($deleted, 'Deleted @number term from !vocabulary.', 'Deleted @number terms from !vocabulary.', array('@number' => $deleted, '!vocabulary' => $vocabulary->name)));
    }
    else {
      drupal_set_message(t('No terms to be deleted.'));
    }
  }

  /**
   * Override parent::configDefaults().
   */
  public function configDefaults() {
    return array(
      'vocabulary' => 0,
      'update_existing' => FEEDS_SKIP_EXISTING,
      'mappings' => array(),
    );
  }

  /**
   * Override parent::configForm().
   */
  public function configForm(&$form_state) {
    $options = array(0 => t('Select a vocabulary'));
    foreach (taxonomy_get_vocabularies() as $vocab) {
      $options[$vocab->machine_name] = check_plain($vocab->name);
    }
    $form = array();
    $form['vocabulary'] = array(
      '#type' => 'select',
      '#title' => t('Import to vocabulary'),
      '#description' => t('Choose the vocabulary to import into. <strong>CAUTION:</strong> when deleting terms through the "Delete items" tab, Feeds will delete <em>all</em> terms from this vocabulary.'),
      '#options' => $options,
      '#default_value' => $this->config['vocabulary'],
    );
    $form['update_existing'] = array(
      '#type' => 'radios',
      '#title' => t('Update existing terms'),
      '#description' => t('Select how existing terms should be updated. Existing terms will be determined using mappings that are a "unique target".'),
      '#options' => array(
        FEEDS_SKIP_EXISTING => 'Do not update existing terms',
        FEEDS_REPLACE_EXISTING => 'Replace existing terms',
        FEEDS_UPDATE_EXISTING => 'Update existing terms (slower than replacing them)',
      ),
      '#default_value' => $this->config['update_existing'],
    );
    return $form;
  }

  /**
   * Override parent::configFormValidate().
   */
  public function configFormValidate(&$values) {
    if (empty($values['vocabulary'])) {
      form_set_error('vocabulary', t('Choose a vocabulary'));
    }
  }

  /**
   * Return available mapping targets.
   */
  public function getMappingTargets() {
    $targets = parent::getMappingTargets();
    $targets += array(
      'name' => array(
        'name' => t('Term name'),
        'description' => t('Name of the taxonomy term.'),
        'optional_unique' => TRUE,
       ),
      'description' => array(
        'name' => t('Term description'),
        'description' => t('Description of the taxonomy term.'),
       ),
    );
    // Let implementers of hook_feeds_term_processor_targets() add their targets.
    if ($vocabulary = $this->vocabulary()) {
      self::loadMappers();
      feeds_alter('feeds_processor_targets', $targets, 'taxonomy_term', $vocabulary->machine_name);
    }
    return $targets;
  }

  /**
   * Get id of an existing feed item term if available.
   */
  protected function existingItemId(FeedsSource $source, FeedsParserResult $result) {
    if ($tid = parent::existingItemId($source, $result)) {
      return $tid;
    }

    // The only possible unique target is name.
    foreach ($this->uniqueTargets($source, $result) as $target => $value) {
      if ($target == 'name') {
        $vocabulary = $this->vocabulary();
        if ($tid = db_query("SELECT tid FROM {taxonomy_term_data} WHERE name = :name AND vid = :vid", array(':name' => $value, ':vid' => $vocabulary->vid))->fetchField()) {
          return $tid;
        }
      }
    }
    return 0;
  }

  /**
   * Return vocabulary to map to.
   */
  public function vocabulary() {
    // Legacy handling for old feeds importers.
    if (is_numeric($this->config['vocabulary'])) {
      $vocabularies = taxonomy_get_vocabularies();
      return isset($vocabularies[$this->config['vocabulary']]) ? $vocabularies[$this->config['vocabulary']] : NULL;
    }
    else {
      if ($vocabulary = taxonomy_vocabulary_machine_name_load($this->config['vocabulary'])) {
        return $vocabulary;
      }
      else {
        $vocabularies = taxonomy_get_vocabularies();
        foreach ($vocabularies as $vocabulary) {
          if ($vocabulary->module == $this->config['vocabulary']) {
            return $vocabulary;
          }
        }
      }
    }
  }

  /**
   * Creates a new term in memory and returns it.
   */
  protected function newTerm($source) {
    $term = new stdClass();
    $this->newItemInfo($term, $source->feed_nid);
    $vocabulary = $this->vocabulary();
    $term->vid = $vocabulary->vid;
    return $term;
  }

  /**
   * Loads an existing term.
   */
  protected function loadTerm($source, $tid) {
    $term = taxonomy_term_load($tid);
    if ($this->loadItemInfo($term)) {
      $this->newItemInfo($term, $source->feed_nid);
    }
    return $term;
  }
}
