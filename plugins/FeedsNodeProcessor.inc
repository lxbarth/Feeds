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
      foreach ($parser_result->value as $value) {
        // @todo
      }
    }
    elseif ($parser_result->type == 'syndication') {
      // @todo
    }
  }

  /**
   * Implementation of FeedsProcessor::purge().
   */
  function purge($nid = NULL) {
    
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

  public function getDefaultConfig() {
    $types = node_get_types('names');
    return array('content_type' => key($types));
  }

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
    return $form;
  }
}