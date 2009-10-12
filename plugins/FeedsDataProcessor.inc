<?php
// $Id$

/**
 * @file
 * Definition of FeedsDataProcessor.
 */

/**
 * Creates simple table records from feed items. Uses Data module.
 */
class FeedsDataProcessor extends FeedsProcessor {

  /**
   * Implementation of FeedsProcessor::process().
   */
  public function process(FeedsParserResult $parserResult, FeedsSource $source) {

    // Count number of created and updated nodes.
    $inserted  = $updated = 0;

    foreach ($parserResult->value['items'] as $item) {
      if (!($id = $this->existingItemId($item, $source)) || $this->config['update_existing']) {
        // Map item to a data record, feed_nid and timestamp are mandatory.
        $data = array();
        $data['feed_nid'] = $source->feed_nid;
        $data['timestamp'] = FEEDS_REQUEST_TIME;
        $data = $this->map($item, $data);

        // Save data.
        if ($id) {
          $data['id'] = $id;
          $this->table()->handler()->update($data, 'id');
          $updated++;
        }
        else {
          $this->table()->handler()->insert($data);
          $inserted++;
        }
      }
    }

    // Set messages.
    if ($inserted) {
      drupal_set_message(t('Created !number records in !table.', array('!number' => $inserted, '!table' => $this->tableName())));
    }
    elseif ($updated) {
      drupal_set_message(t('Updated !number records in !table.', array('!number' => $updated, '!table' => $this->tableName())));
    }
    else {
      drupal_set_message(t('There are no new records.'));
    }
  }

  /**
   * Implementation of FeedsProcessor::purge().
   *
   * Delete all data records for feed_nid in this table.
   */
  public function purge(FeedsSource $source) {
    $num = $this->table()->handler()->delete(array('feed_nid' => $source->feed_nid));
    drupal_set_message(t('Deleted !number records from !table.', array('!number' => $num, '!table' => $this->tableName())));
  }

  /**
   * Override parent::addMapping() and create a new field for new mapping
   * targets.
   *
   * @see getMappingTargets().
   */
  public function addMapping($source, $target, $unique = FALSE) {
    if (empty($source) || empty($target)) {
      return;
    }

    // Create a new field with targets that start with "new:"
    @list($new, $type) = explode(':', $target);

    if ($new == 'new') {
      // Build a field name from the source key.
      $field_name = data_safe_name($source);
      // Get the full schema spec from data.
      $type = data_get_field_definition($type);
      // Add the field to the table.
      $schema = $this->table()->get('table_schema');
      if (!isset($schema['fields'][$field_name])) {
        $target = $this->table()->addField($field_name, $type);
        // Let the user know.
        drupal_set_message(t('Created new field "!name".', array('!name' => $field_name)));
      }
      else {
        throw new Exception(t('Field !field_name already exists as a mapping target. Remove it from mapping if you would like to map another source to it. Remove it from !data_table table if you would like to change its definition.', array('!field_name' => $field_name, '!data_table' => l($this->table()->get('name'), 'admin/content/data'))));
      }
    }

    // Let parent populate the mapping configuration.
    parent::addMapping($source, $target, $unique);
  }

  /**
   * Return available mapping targets.
   */
  public function getMappingTargets() {
    $schema = $this->table()->get('table_schema');
    $meta = $this->table()->get('meta');

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
      $new_fields['new:'. $k] = t('[new] !type', array('!type' => $v));
    }
    return $new_fields + $existing_fields;
  }

  /**
   * Iterate through unique targets and try to load existing records.
   * Return id for the first match.
   */
  protected function existingItemId($source_item, FeedsSource $source) {
    foreach ($this->uniqueTargets($source_item) as $target => $value) {
      if ($records = $this->table()->handler()->load(array($target => $value))) {
        return $records[0]['id'];
      }
    }
    return 0;
  }

  /**
   * Override parent::configDefaults().
   */
  public function configDefaults() {
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
   * @todo: Make *Data module* throw exception when table can't be found or
   *   can't be created.
   */
  protected function table() {
    if ($table = data_get_table($this->tableName())) {
      return $table;
    }
    else {
      if ($table = data_create_table($this->tableName(), $this->baseSchema(), feeds_importer($this->id)->config['name'])) {
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

/**
 * Simple multidimensional data handler. Treats tables that join to this
 * handler's table through FeedsDatahandler::key as a cluster. Records in this
 * cluster are regarded as belonging to one multidimensional data set joined
 * by FeedsDatahandler::key.
 *
 * Limitations:
 *
 * - Records can only be tied together by a single key. Note: tables can still
 *   join through other fields to this table, but these table's data won't be
 *   considered of the same data set.
 * - save() is not supported. update() only supports updates on
 *   FeedsDataHandler::key
 */
class FeedsDataHandler extends DataHandler {

  // An array of tables joining to the base table.
  protected $joined_tables;
  // A single field that the base table ($this->table) and depending tables
  // join on.
  protected $key;

  /**
   * Constructor, call indirectly through DataHandler::instance();
   */
  protected function __construct($table, $key) {
    $this->table = $table;
    $this->key = $key;

    // Find tables joining to this table.
    // @todo DB cache.
    $this->joined_tables = array();
    $tables = data_get_all_tables();
    foreach ($tables as $join_table) {
      if($join_table->get('name') == $this->table) {
        // don't bother with joins on the same table...
        continue;
      }
      $meta = $join_table->get('meta');
      $joins = $meta['join'];
      if (isset($joins[$this->table]) && $joins[$this->table]['left_field'] == $this->key) {
        // table has a field that joins to this table on $this->key
        $this->joined_tables[$join_table->get('name')] = $join_table->get('name');
      }
    }
  }

  /**
   * Instantiate a FeedsDataHandler object.
   *
   * @param $table
   *   The name of the table to access with this DataHandler object.
   * @param $key
   *   The key that joins other tables.
   */
  public static function instance($table, $key) {
    static $handlers;
    if (!isset($handlers[$table][$key])) {
      $handlers[$table][$key] = new FeedsDataHandler($table, $key);
    }
    return $handlers[$table][$key];
  }

  /**
   * Inserts a multi dimensional record.
   *
   * @param $record
   *   An array of a record to store. Keys are the names of fields or names of
   *   joining tables.
   */
  public function insert(&$record) {
    $base_record = array();
    foreach ($records as $k => $v) {
      if (in_array($k, $this->joined_tables)) {
        parent::insert($v);
      }
      else {
        $base_record[$k] = $v;
      }
    }
    parent::insert($base_record);
  }

  /**
   * Updates a multi-dimensional record.
   *
   * Assumes that updates occur on keys. Does not support any other form of
   * updates.
   *
   * @param $record
   *   An array of the record to update. Keys are the names of fields or names
   *   of joining tables. At least one key name in $record must match
   *   $this->key.
   */
  public function update(&$record) {
    $base_record = array();
    foreach ($records as $k => $v) {
      if (in_array($k, $this->joined_tables)) {
        parent::update($v, $this->key);
      }
      else {
        $base_record[$k] = $v;
      }
    }
    parent::update($base_record, $this->key);
  }

  /**
   * Does not support save(). protected.
   */
  protected function save(&$record, $update) {
    parent::save($record, $update);
  }

  /**
   * Delete records.
   */
  public function delete($keys) {

    // First delete records from joined tables.
    foreach ($this->joined_table as $table) {
      $where = $values = array();
      $schema = drupal_get_schema($this->table);
      $fields = $schema['fields'];
      foreach ($keys as $key => $value) {
        $where[] = 'base.'. db_escape_string($key) ." = ". db_type_placeholder($fields[$key]['type']);
        $values[] = $value;
      }
      if (!empty($where)) {
        db_query('DELETE {'. db_escape_table($table) .'} joined FROM {'. db_escape_table($this->table) .'} base JOIN  {'. db_escape_table($table) .'} joined ON base.'. $this->key .' = joined.'. $this->key .' WHERE '. implode(' AND ', $where), $values);
      }
    }
    return parent::delete($keys);
  }
}