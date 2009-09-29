<?php
// $Id: FeedsNodeProcessor.inc,v 1.2 2009/09/19 15:52:46 alexb Exp $

class FeedsNodeProcessor extends FeedsProcessor {

  function process($feed) {
    foreach ($feed->items as $item) {
      if ($this->unique($item)) {
        $node = $this->map($item);
        node_save($node);
      }
    }
  }

  /**
   * Return available mapping targets.
   */
  function getMappingTargets() {
    return array(
      'title' => FALSE, 
      'body' => FALSE, 
      'status' => FALSE, 
      'url' => TRUE, 
      'guid' => TRUE,
    );
  }
}