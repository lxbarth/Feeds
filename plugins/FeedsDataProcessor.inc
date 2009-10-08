<?php
// $Id$

/**
 * @file
 * Definition of FeedsDataProcessor.
 */

/**
 *
 */
class FeedsDataProcessor extends FeedsProcessor {

  /**
   * Implementation of FeedsProcessor::process().
   */
  public function process(FeedsParserResult $parserResult, FeedsSource $source) {

    // Count number of created and updated nodes.
    $created  = $updated = 0;

    foreach ($parserResult->value['items'] as $item) {
      if (!$existing = $this->existingItemId($item, $source) || $this->config['update_existing']) {

        // Map item to a data record.
        $data = $this->map($item);

        // Only pass in $update array if we know we are updating.
        // Otherwise DataHandler executes an unnecessary check.
        $update = array();
        if ($this->config['update_existing']) {
          $update = $this->uniqueFields();
        }

        // Save data.
        $this->table()->handler()->save($data, $update);

        if ($existing) {
          $updated++;
        }
        else {
          $created++;
        }
      }
    }

    // Set messages.
    if ($created) {
      drupal_set_message(t('Created !number records in !table.', array('!number' => $created, '!table' => $this->tableName())));
    }
    elseif ($updated) {
      drupal_set_message(t('Updated !number records in !table.', array('!number' => $updated, '!table' => $this->tableName())));
    }
    else {
      drupal_set_message(t('There are no new records.'));
    }
  }

  /**
   * Implementation of FeedsProcesor::purge().
   *
   * @todo
   */
  public function purge(FeedsSource $source) {

  }

  /**
   * Return available mapping targets.
   */
  public function getMappingTargets() {
    $schema = $this->table()->get('table_schema');
    $meta = $schema = $this->table()->get('meta');

    // Collect all existing fields except id and field_nid and offer them as
    // mapping targets.
    $existing_fields = $new_fields = array();
    if (isset($schema['fields'])) {
      foreach ($schema['fields'] as $field_name => $field) {
        if (!in_array($field_name, array('id', 'feed_nid'))) {
          $existing_fields[$field_name] = empty($meta['fields'][$field_name]['label']) ? $field_name : $meta['fields'][$field_name]['label'];
        }
      }
    }

    // Now add data field types as mapping targets.
    $field_types = drupal_map_assoc(array_keys(data_get_field_definitions()));
    foreach ($field_types as $k => $v) {
      $new_fields['new:'. $k] = t('New !type', array('!type' => $v));
    }
    return $new_fields + $existing_fields;
  }

  /**
   * Override setTargetElement to operate on a target item that is a data record.
   */
  public function setTargetElement($target_item, $target_element, $value) {
    // @todo.
  }

  public function existingItemId($source_item, FeedsSource $source) {

  }

  /**
   * Override parent::getDefaultConfig().
   */
  public function getDefaultConfig() {
    return array(
      'update_existing' => FALSE,
      'mappings' => array(),
    );
  }

  /**
   * Override parent::configForm().
   */
  public function configForm(&$form_state) {
    $form['update_existing'] = array(
      '#type' => 'checkbox',
      '#title' => t('Update existing items'),
      '#description' => t('Check if existing items should be updated from the feed.'),
      '#default_value' => $this->config['update_existing'],
    );
    return $form;
  }

  /**
   * Return the data table name for this feed.
   */
  protected function tableName() {
    return 'feeds_data_'. $this->id;
  }

  /**
   * Return the data table for this feed.
   *
   * @throws Exception $e
   *   Throws this exception if a table cannot be found and cannot be created.
   *
   * @todo: Make *Data module* throw exceptions when table can't be found or
   *   can't be created.
   */
  protected function table() {
    if ($table = data_get_table($this->tableName())) {
      return $table;
    }
    else {
      if ($table = data_create_table($this->tableName(), $this->baseSchema(), feeds_feed($this->id)->config['name'])) {
        return $table;
      }
    }
    throw new Exception('Could not create data table.');
  }

  /**
   * Every Feeds data table must have these elements.
   */
  protected function baseSchema() {
    return array(
      'fields' => array(
        'feed_nid' => array(
          'type' => 'int',
          'unsigned' => TRUE,
          'not null' => TRUE,
        ),
        'id' => array(
          'type' => 'serial',
          'size' => 'normal',
          'unsigned' => TRUE,
          'not null' => TRUE,
        ),
        'timestamp' => array(
          'description' => 'The Unix timestamp for the data.',
          'type' => 'int',
          'unsigned' => TRUE,
          'not null' => FALSE,
        ),
      ),
      'indexes' => array(
        'feed_nid' => array('feed_nid'),
        'id' => array('id'),
        'timestamp' => array('timestamp'),
       ),
       'primary key' => array(
         '0' => 'id',
       ),
    );
  }
}