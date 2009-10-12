<?php
// $Id$

/**
 * @file
 * Data handler used in FeedsDataProcessor.
 */

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
 * - Note: a record in depending tables will deleted when records from the base
 *   table are deleted.
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
   * Delete records from this handler's base table and all joined tables.
   */
  public function delete($keys) {

    // Assemble where clause for delete query.
    $where = $values = array();
    foreach ($keys as $key => $value) {
      $where[] = 'base.'. db_escape_string($key) ." = ". db_type_placeholder($fields[$key]['type']);
      $values[] = $value;
    }

    // Only execute delete statement if where clause has been populated.
    if (!empty($where)) {

      // First delete records from joined tables, then delete this table's
      // record.
      foreach ($this->joined_table as $table) {
        $schema = drupal_get_schema($this->table);
        $fields = $schema['fields'];
        if (!empty($where)) {
          db_query('DELETE joined FROM {'. db_escape_table($this->table) .'} base JOIN  {'. db_escape_table($table) .'} joined ON base.'. $this->key .' = joined.'. $this->key .' WHERE '. implode(' AND ', $where), $values);
        }
      }
      db_query('DELETE base FROM {'. db_escape_table($this->table) .'} base WHERE '. implode(' AND ', $where), $values);
    }

    return db_affected_rows();
  }
}