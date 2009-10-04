<?php
// $Id: FeedsNodeProcessor.inc,v 1.2 2009/09/19 15:52:46 alexb Exp $
/**
 * @file
 * Class definition of FeedsNodeProcessor.
 */

/**
 * Creates nodes from feed items.
 */
class FeedsNodeProcessor extends FeedsProcessor {

  /**
   * Implementation of FeedsProcessor::process();
   */
  function process(FeedsParserResult $parserResult, $nid = NULL) {
    if ($parser_result->type == 'simple') {
    }
    elseif ($parser_result->type == 'syndication') {
    }
    foreach ($parser_result->value['items'] as $item) {
      $node = $this->map($item);
      dsm($node);
    }
  }

  /**
   * Implementation of FeedsProcessor::purge().
   */
  function purge($nid = NULL) {
    // @todo
  }

  /**
   * Return available mapping targets.
   */
  function getMappingTargets() {
    $targets = array(
      'title' => array(),
      'body' => array(), 
      'status' => array(),
      'url' => array(
        'optional_unique' => TRUE, 
        ),
      'guid' => array(
        'optional_unique' => TRUE, 
        ),
        /* @todo: move to mapper api.
      'taxonomy' => array(
        'options' => taxonomy_get_vocabularies(),
        ),
        */
    );
    drupal_alter('feeds_node_processor_targets', $targets);
    return $targets;
  }

  /**
   * Execute mapping on an item.
   */
  public function map($source_item) {
    // Prepare node object.
    $target_node = new stdClass();
    $target_node->type = $this->config['content_type'];
    node_object_prepare($target_node);

    // Have parent class do the iterating.
    return parent::map($source_item, $target_node);
  }

  /**
   * Override setTargetElement to operate on a target item that is a node.
   */
  public function setTargetElement($target_node, $target_element, $value) {
    $target_node->$target_element = $value;
  }
  
  /**
   * Override parent::getDefaultConfig().
   */
  public function getDefaultConfig() {
    $types = node_get_types('names');
    return array('content_type' => key($types), 'update_existing' => FALSE);
  }

  /**
   * Override parent::configForm().
   */
  public function configForm(&$form_state) {
    $types = node_get_types('names');
    $form = array();
    $form['content_type'] = array(
      '#type' => 'select',
      '#title' => t('Content type'),
      '#description' => t('Choose node type to create from this feed.'),
      '#options' => $types,
      '#default_value' => $this->config['content_type'],
    );
    $form['update_existing'] = array(
      '#type' => 'checkbox',
      '#title' => t('Update existing items'),
      '#description' => t('Check if existing items should be updated from the feed.'),
      '#default_value' => $this->config['update_existing'],
    );
    return $form;
  }
}