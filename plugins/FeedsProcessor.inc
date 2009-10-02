<?php
// $Id$

/**
 * Abstract class, defines interface for processors.
 */
abstract class FeedsProcessor extends FeedsConfigurable {

  /**
   * Process the result of the parser or previous processors.
   * Extending classes must implement this method.
   */
  public abstract function process(FeedsParserResult $parser_result);

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
    $this->save();
  }

  /**
   * Remove a mapping.
   */
  public function removeMapping($target) {
    unset($this->config['mappings'][$target]);
    $this->save();
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
    // @todo: invoke hook_feeds_mapper() here.
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