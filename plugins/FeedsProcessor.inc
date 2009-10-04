<?php
// $Id$

/**
 * Abstract class, defines interface for processors.
 */
abstract class FeedsProcessor extends FeedsConfigurable {

  /**
   * Process the result of the parser or previous processors.
   * Extending classes must implement this method.
   * 
   * @param FeedsParserResult $parserResult
   *   The result of the parsing stage.
   * @param $nid
   *   Node ID of a feed node, IF this configuration is attached to a node.
   */
  public abstract function process(FeedsParserResult $parserResult, $nid = NULL);

  /**
   * Remove all stored results for this configuration OR - if $nid given -
   * for this configuration for this feed node.
   */
  public abstract function purge($nid = NULL);

  /**
   * Declare default configuration.
   */
  public function getDefaultConfig() {
    return array('mappings' => FALSE);
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
    $parser = feeds_instance('Feed', $this->id)->parser;
    if (empty($target_item)) {
      $target_item = array();
    }

    foreach ($this->config['mappings'] as $mapping) {
      $value = $parser->getSourceElement($source_item, $mapping['sources']);
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
   * Determine whether a given item is unique.
   */
  public function unique($item) {
    // @todo.
  }
}