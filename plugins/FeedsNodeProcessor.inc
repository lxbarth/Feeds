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
   * Implementation of FeedsProcessor::process().
   */
  public function process(FeedsParserResult $parserResult, $feed_nid = NULL) {

    foreach ($parserResult->value['items'] as $item) {
      // If the target item does not exist OR if update_existing is enabled, map and save.
      if (!$nid = $this->existingItemId($item, $feed_nid) || $this->config['update_existing']) {

        // Map item to a node.
        $node = $this->map($item);

        // Add some default information.
        $node->feeds_node_item->id = $this->id;
        $node->feeds_node_item->imported = FEEDS_REQUEST_TIME;
        $node->feeds_node_item->feed_nid = $feed_nid;
        if (!empty($nid)) {
          $node->nid = $nid;
        }

        // Save the node.
        node_save($node);
      }
    }
  }

  /**
   * Implementation of FeedsProcessor::purge().
   * @todo: use batch API.
   */
  public function purge($feed_nid = NULL) {
    $result = db_query('SELECT nid FROM {feeds_node_item} WHERE feed_nid = %d', $feed_nid);
    while ($node = db_fetch_object($result)) {
      node_delete($node->nid);
    }
  }

  /**
   * Return available mapping targets.
   */
  public function getMappingTargets() {
    $targets = array(
      'title' => array(
        'name' => t('Node title'), // @todo: write out.
       ),
      'status' => array(
        'name' => t('Published status'),
      ),
      'created' => array(
        'name' => t('Published date'),
      ),
      'body' => array(
        'name' => t('Node body'),
      ),
      'url' => array(
        'name' => t('URL'),
        'optional_unique' => TRUE,
        ),
      'guid' => array(
        'name' => t('GUID'),
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
    static $included;
    if (!$included) {
      module_load_include('inc', 'node', 'node.pages');
      $included = TRUE;
    }
    $target_node = new stdClass();
    $target_node->type = $this->config['content_type'];
    $target_node->feeds_node_item = new stdClass();
    node_object_prepare($target_node);

    // Have parent class do the iterating.
    return parent::map($source_item, $target_node);
  }

  /**
   * Override setTargetElement to operate on a target item that is a node.
   */
  public function setTargetElement($target_node, $target_element, $value) {
    if (in_array($target_element, array('url', 'guid'))) {
      $target_node->feeds_node_item->$target_element = $value;
    }
    elseif (in_array($target_element, array('title', 'body', 'status', 'created'))) {
      $target_node->$target_element = $value;
    }
  }

  /**
   * Get nid of an existing feed item node if available.
   */
  public function existingItemId($source_item, $feed_nid) {
    $parser = feeds_instance('Feed', $this->id)->parser;
    $targets = array();
    foreach ($this->config['mappings'] as $mapping) {
      if ($mapping['unique']) {
        $targets[$target] = $parser->getSourceElement($source_item, $mapping['source']);
      }
    }
    foreach ($targets as $target => $value) {
      switch ($targets) {
        case 'url':
          $nid = db_result(db_query('SELECT nid FROM {feeds_node_item} WHERE feed_nid = %d AND url = "%s"', $feed_nid, $value));
          break;
        case 'guid':
          $nid = db_result(db_query('SELECT nid FROM {feeds_node_item} WHERE feed_nid = %d AND guid = "%s"', $feed_nid, $value));
          break;
      }
      if ($nid) {
        // Return with the first nid found.
        return $nid;
      }
    }
    return FALSE;
  }

  /**
   * Override parent::getDefaultConfig().
   */
  public function getDefaultConfig() {
    $types = node_get_types('names');
    return array('content_type' => key($types), 'update_existing' => FALSE, 'mappings' => array());
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