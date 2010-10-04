<?php
// $Id: FeedsProcessor.inc,v 1.20.2.3 2010/10/04 01:33:43 alexb Exp $


// Update mode for existing items.
define('FEEDS_SKIP_EXISTING', 0);
define('FEEDS_REPLACE_EXISTING', 1);
define('FEEDS_UPDATE_EXISTING', 2);

// Default limit for creating items on a page load, not respected by all
// processors.
define('FEEDS_PROCESS_LIMIT', 50);

/**
 * Abstract class, defines interface for processors.
 */
abstract class FeedsProcessor extends FeedsPlugin {
  /**
   * Entity type this processor operates on.
   */
  protected $entity_type;

  /**
   * Process the result of the parser or previous processors.
   * Extending classes must implement this method.
   *
   * @param FeedsSource $source
   *   Source information about this import.
   * @param FeedsParserResult $parser_result
   *   The result of the parsing stage.
   */
  public abstract function process(FeedsSource $source, FeedsParserResult $parser_result);

  /**
   * Remove all stored results or stored results up to a certain time for this
   * configuration/this source.
   *
   * @param FeedsSource $source
   *   Source information for this expiry. Implementers should only delete items
   *   pertaining to this source. The preferred way of determining whether an
   *   item pertains to a certain souce is by using $source->feed_nid. It is the
   *   processor's responsibility to store the feed_nid of an imported item in
   *   the processing stage.
   */
  public abstract function clear(FeedsSource $source);

  /*
   * Report number of items that can be processed per call.
   *
   * 0 means 'unlimited'.
   *
   * If a number other than 0 is given, Feeds parsers that support batching
   * will only deliver this limit to the processor.
   *
   * @see FeedsSource::getLimit()
   * @see FeedsCSVParser::parse()
   * @see FeedsNodeProcessor::getLimit()
   */
  public function getLimit() {
    return 0;
  }

  /**
   * Delete feed items younger than now - $time. Do not invoke expire on a
   * processor directly, but use FeedsImporter::expire() instead.
   *
   * @see FeedsImporter::expire().
   * @see FeedsDataProcessor::expire().
   *
   * @param $time
   *   If implemented, all items produced by this configuration that are older
   *   than REQUEST_TIME - $time should be deleted.
   *   If $time === NULL processor should use internal configuration.
   *
   * @return
   *   FEEDS_BATCH_COMPLETE if all items have been processed, a float between 0
   *   and 0.99* indicating progress otherwise.
   */
  public function expire($time = NULL) {
    return FEEDS_BATCH_COMPLETE;
  }

  /**
   * Counts the number of items imported by this processor.
   */
  public function itemCount(FeedsSource $source) {
    return db_query("SELECT count(*) FROM {feeds_item} WHERE entity_type = :entity_type AND feed_nid = :feed_nid", array(':entity_type' => $this->entity_type, ':feed_nid' => $source->feed_nid))->fetchField();
  }

  /**
   * Execute mapping on an item.
   *
   * This method encapsulates the central mapping functionality. When an item is
   * processed, it is passed through map() where the properties of $source_item
   * are mapped onto $target_item following the processor's mapping
   * configuration.
   *
   * For each mapping FeedsParser::getSourceElement() is executed to retrieve
   * the source element, then FeedsProcessor::setTargetElement() is invoked
   * to populate the target item properly. Alternatively a
   * hook_x_targets_alter() may have specified a callback for a mapping target
   * in which case the callback is asked to populate the target item instead of
   * FeedsProcessor::setTargetElement().
   *
   * @ingroup mappingapi
   *
   * @see hook_feeds_parser_sources_alter()
   * @see hook_feeds_data_processor_targets_alter()
   * @see hook_feeds_node_processor_targets_alter()
   * @see hook_feeds_term_processor_targets_alter()
   * @see hook_feeds_user_processor_targets_alter()
   */
  protected function map(FeedsSource $source, FeedsParserResult $result, $target_item = NULL) {

    // Static cache $targets as getMappingTargets() may be an expensive method.
    static $sources;
    if (!isset($sources[$this->id])) {
      $sources[$this->id] = feeds_importer($this->id)->parser->getMappingSources();
    }
    static $targets;
    if (!isset($targets[$this->id])) {
      $targets[$this->id] = $this->getMappingTargets();
    }
    $parser = feeds_importer($this->id)->parser;
    if (empty($target_item)) {
      $target_item = array();
    }

    // Many mappers add to existing fields rather than replacing them. Hence we
    // need to clear target elements of each item before mapping in case we are
    // mapping on a prepopulated item such as an existing node.
    foreach ($this->config['mappings'] as $mapping) {
      if (isset($targets[$mapping['target']]['real_target'])) {
        unset($target_item->{$targets[$mapping['target']]['real_target']});
      }
      elseif (isset($target_item->{$mapping['target']})) {
        unset($target_item->{$mapping['target']});
      }
    }

    /*
    This is where the actual mapping happens: For every mapping we envoke
    the parser's getSourceElement() method to retrieve the value of the source
    element and pass it to the processor's setTargetElement() to stick it
    on the right place of the target item.

    If the mapping specifies a callback method, use the callback instead of
    setTargetElement().
    */
    self::loadMappers();
    foreach ($this->config['mappings'] as $mapping) {
      // Retrieve source element's value from parser.
      if (is_array($sources[$this->id][$mapping['source']]) &&
          isset($sources[$this->id][$mapping['source']]['callback']) &&
          function_exists($sources[$this->id][$mapping['source']]['callback'])) {
        $callback = $sources[$this->id][$mapping['source']]['callback'];
        $value = $callback($source, $result, $mapping['source']);
      }
      else {
        $value = $parser->getSourceElement($source, $result, $mapping['source']);
      }

      // Map the source element's value to the target.
      if (is_array($targets[$this->id][$mapping['target']]) &&
          isset($targets[$this->id][$mapping['target']]['callback']) &&
          function_exists($targets[$this->id][$mapping['target']]['callback'])) {
        $callback = $targets[$this->id][$mapping['target']]['callback'];
        $callback($target_item, $mapping['target'], $value);
      }
      else {
        $this->setTargetElement($target_item, $mapping['target'], $value);
      }
    }
    return $target_item;
  }

  /**
   * Per default, don't support expiry. If processor supports expiry of imported
   * items, return the time after which items should be removed.
   */
  public function expiryTime() {
    return FEEDS_EXPIRE_NEVER;
  }

  /**
   * Declare default configuration.
   */
  public function configDefaults() {
    return array('mappings' => array());
  }

  /**
   * Get mappings.
   */
  public function getMappings() {
    return isset($this->config['mappings']) ? $this->config['mappings'] : array();
  }

  /**
   * Declare possible mapping targets that this processor exposes.
   *
   * @ingroup mappingapi
   *
   * @return
   *   An array of mapping targets. Keys are paths to targets
   *   separated by ->, values are TRUE if target can be unique,
   *   FALSE otherwise.
   */
  public function getMappingTargets() {
    return array(
      'url' => array(
        'name' => t('URL'),
        'description' => t('The external URL of the item. E. g. the feed item URL in the case of a syndication feed. May be unique.'),
        'optional_unique' => TRUE,
      ),
      'guid' => array(
        'name' => t('GUID'),
        'description' => t('The globally unique identifier of the item. E. g. the feed item GUID in the case of a syndication feed. May be unique.'),
        'optional_unique' => TRUE,
      ),
    );
  }

  /**
   * Set a concrete target element. Invoked from FeedsProcessor::map().
   *
   * @ingroup mappingapi
   */
  public function setTargetElement($target_item, $target_element, $value) {
    switch ($target_element) {
      case 'url':
      case 'guid':
        $target_item->feeds_item->$target_element = $value;
        break;
      default:
        $target_item->$target_element = $value;
        break;
    }
  }

  /**
   * Retrieve the target item's existing id if available. Otherwise return 0.
   *
   * @ingroup mappingapi
   *
   * @param FeedsSource $source
   *   The source information about this import.
   * @param $result
   *   A FeedsParserResult object.
   *
   * @return
   *   The serial id of an entity if found, 0 otherwise.
   */
  protected function existingItemId(FeedsSource $source, FeedsParserResult $result) {
    $query = db_select('feeds_item')
      ->fields('feeds_item', array('entity_id'))
      ->condition('feed_nid', $source->feed_nid)
      ->condition('entity_type', $this->entity_type)
      ->condition('id', $source->id);

    // Iterate through all unique targets and test whether they do already
    // exist in the database.
    foreach ($this->uniqueTargets($source, $result) as $target => $value) {
      switch ($target) {
        case 'url':
          $entity_id = $query->condition('url', $value)->execute()->fetchField();
          break;
        case 'guid':
          $entity_id = $query->condition('guid', $value)->execute()->fetchField();
          break;
      }
      if (isset($entity_id)) {
        // Return with the content id found.
        return $entity_id;
      }
    }
    return 0;
  }


  /**
   * Utility function that iterates over a target array and retrieves all
   * sources that are unique.
   *
   * @param $batch
   *   A FeedsImportBatch.
   *
   * @return
   *   An array where the keys are target field names and the values are the
   *   elements from the source item mapped to these targets.
   */
  protected function uniqueTargets(FeedsSource $source, FeedsParserResult $result) {
    $parser = feeds_importer($this->id)->parser;
    $targets = array();
    foreach ($this->config['mappings'] as $mapping) {
      if ($mapping['unique']) {
        // Invoke the parser's getSourceElement to retrieve the value for this
        // mapping's source.
        $targets[$mapping['target']] = $parser->getSourceElement($source, $result, $mapping['source']);
      }
    }
    return $targets;
  }

  /**
   * Adds Feeds specific information on $entity->feeds_item.
   *
   * @param $entity
   *   The entity object to be populated with new item info.
   * @param $feed_nid
   *   The feed nid of the source that produces this entity.
   * @param $hash
   *   The fingerprint of the source item.
   */
  protected function newItemInfo($entity, $feed_nid, $hash = '') {
    $entity->feeds_item = new stdClass();
    $entity->feeds_item->entity_id = 0;
    $entity->feeds_item->entity_type = $this->entity_type;
    $entity->feeds_item->id = $this->id;
    $entity->feeds_item->feed_nid = $feed_nid;
    $entity->feeds_item->imported = REQUEST_TIME;
    $entity->feeds_item->hash = $hash;
    $entity->feeds_item->url = '';
    $entity->feeds_item->guid = '';
  }

  /**
   * Loads existing entity information and places it on $entity->feeds_item.
   *
   * @param $entity
   *   The entity object to load item info for. Id key must be present.
   *
   * @return
   *   TRUE if item info could be loaded, false if not.
   */
  protected function loadItemInfo($entity) {
    $entity_info = entity_get_info($this->entity_type);
    $key = $entity_info['entity keys']['id'];
    if ($item_info = feeds_item_info_load($this->entity_type, $entity->$key)) {
      $entity->feeds_item = $item_info;
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Create MD5 hash of item and mappings array.
   *
   * Include mappings as a change in mappings may have an affect on the item
   * produced.
   *
   * @return Always returns a hash, even with empty, NULL, FALSE:
   *  Empty arrays return 40cd750bba9870f18aada2478b24840a
   *  Empty/NULL/FALSE strings return d41d8cd98f00b204e9800998ecf8427e
   */
  protected function hash($item) {
    static $serialized_mappings;
    if (!$serialized_mappings) {
      $serialized_mappings = serialize($this->config['mappings']);
    }
    return hash('md5', serialize($item) . $serialized_mappings);
  }

  /**
   * Retrieve MD5 hash of $nid from DB.
   * @return Empty string if no item is found, hash otherwise.
   */
  protected function getHash($entity_id) {
    if ($hash = db_query("SELECT hash FROM {feeds_item} WHERE entity_type = :type AND entity_id = :id", array(':type' => $this->entity_type, ':id' => $entity_id))->fetchField()) {
      // Return with the hash.
      return $hash;
    }
    return '';
  }
}
