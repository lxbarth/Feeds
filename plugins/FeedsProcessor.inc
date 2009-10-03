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
  public function addMapping($source, $target, $unique = NULL) {
    if (!empty($source) && !empty($target)) {
      $this->config['mappings'][$target] = array(
        'source' => $source,
        'unique' => $unique,
      );
    }
  }

  /**
   * Remove a mapping.
   */
  public function removeMapping($target) {
    unset($this->config['mappings'][$target]);
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
  public function map($item) {
    // @todo.
  }

  /**
   * Determine whether a given item is unique.
   */
  public function unique($item) {
    // @todo.
  }
}