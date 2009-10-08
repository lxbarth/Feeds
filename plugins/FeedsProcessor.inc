<?php
// $Id$

/**
 * Abstract class, defines interface for processors.
 *
 * @todo: make implement FeedsSourceInterface
 */
abstract class FeedsProcessor extends FeedsPlugin {

  /**
   * Process the result of the parser or previous processors.
   * Extending classes must implement this method.
   *
   * @param FeedsParserResult $parserResult
   *   The result of the parsing stage.
   * @param FeedsSource $source
   *   Source information about this import.
   */
  public abstract function process(FeedsParserResult $parserResult, FeedsSource $source);

  /**
   * Remove all stored results for this configuration/this source.
   */
  public abstract function purge(FeedsSource $source);

  /**
   * Declare default configuration.
   */
  public function getDefaultConfig() {
    return array('mappings' => array());
  }

  /**
   * Add a mapping to existing mappings.
   */
  public function addMapping($source, $target, $unique = FALSE) {
    if (!empty($source) && !empty($target)) {
      $this->config['mappings'][] = array(
        'source' => $source,
        'target' => $target,
        'unique' => $unique,
      );
    }
  }

  /**
   * Set unique state of a mapping target.
   */
  public function setUnique($source, $target, $unique) {
    if (!empty($source) && !empty($target)) {
      foreach ($this->config['mappings'] as $k => $mapping) {
        if ($mapping['source'] == $source && $mapping['target'] == $target) {
          $this->config['mappings'][$k]['unique'] = $unique;
        }
      }
    }
  }

  /**
   * Remove a mapping.
   */
  public function removeMapping($source, $target) {
    foreach ($this->config['mappings'] as $k => $mapping) {
      if ($mapping['source'] == $source && $mapping['target'] == $target) {
        unset($this->config['mappings'][$k]);
      }
    }
    // Keep or keys clean.
    $this->config['mappings'] = array_values($this->config['mappings']);
  }

  /**
   * Declare possible mapping targets.
   *
   * @return
   *   An array of mapping targets. Keys are paths to targets
   *   separated by ->, values are TRUE if target can be unique,
   *   FALSE otherwise.
   */
  public function getMappingTargets() {
    return array();
  }

  /**
   * Get mappings.
   */
  public function getMappings() {
    return $this->config['mappings'];
  }

  /**
   * Execute mapping on an item.
   */
  public function map($source_item, $target_item = NULL) {
    $parser = feeds_feed($this->id)->parser;
    if (empty($target_item)) {
      $target_item = array();
    }

    // This is where the actual mapping happens: For every mapping we envoke
    // the parser's getSourceElement() method to retrieve the value of the source
    // element and pass it to the processor's setTargetElement() to stick it
    // on the right place of the target item.
    foreach ($this->config['mappings'] as $mapping) {
      $value = $parser->getSourceElement($source_item, $mapping['source']);
      $this->setTargetElement($target_item, $mapping['target'], $value);
    }
    return $target_item;
  }

  /**
   * Set target element.
   */
  public function setTargetElement(&$target_item, $target_element, $value) {
    $target_item[$target_element] = $value;
  }

  /**
   * Retrieve the target item's existing id if available. Otherwise return FALSE.
   *
   * @param $source_item
   *   A single item that has been aggregated from a feed.
   * @param FeedsSource $source
   *   The source information about this import.
   */
  public function existingItemId($source_item, FeedsSource $source) {
    return FALSE;
  }
}