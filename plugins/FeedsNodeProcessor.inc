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
   * Override access method. Check whether user has sufficient permissions to
   * create and delete the type of node that this processor generates.
   */
  public function access($action, $account) {
    switch ($action) {
      case 'import':
        return node_access('create', $this->config['content_type'], $account);
      case 'purge':
        // Only users with administer nodes permissions can delete nodes.
        // We have no way of knowing the actual content types of the nodes attached to
        // this feed configuration.
        return user_access('administer nodes', $account);
    }
    // Fine for potential other actions.
    return TRUE;
  }

  /**
   * Implementation of FeedsProcessor::process().
   */
  public function process(FeedsParserResult $parserResult, FeedsSource $source) {

    // Count number of created and updated nodes.
    $created  = $updated = 0;

    foreach ($parserResult->value['items'] as $item) {
      // If the target item does not exist OR if update_existing is enabled, map and save.
      if (!$nid = $this->existingItemId($item, $source) || $this->config['update_existing']) {

        // Map item to a node.
        $node = $this->map($item);

        // Add some default information.
        $node->feeds_node_item->id = $this->id;
        $node->feeds_node_item->imported = FEEDS_REQUEST_TIME;
        $node->feeds_node_item->feed_nid = $source->feed_nid;
        if (!empty($nid)) {
          $node->nid = $nid;
        }

        // Save the node.
        node_save($node);

        if ($nid) {
          $updated++;
        }
        else {
          $created++;
        }
      }
    }

    // Set messages.
    if ($created) {
      drupal_set_message(t('Created !number !type nodes.', array('!number' => $created, '!type' => $this->config['content_type'])));
    }
    elseif ($updated) {
      drupal_set_message(t('Updated !number !type nodes.', array('!number' => $updated, '!type' => $this->config['content_type'])));
    }
    else {
      drupal_set_message(t('There is no new content.'));
    }
  }

  /**
   * Implementation of FeedsProcessor::purge().
   * @todo: use batch API.
   */
  public function purge(FeedsSource $source) {
    // Count number of deleted nodes.
    $deleted = 0;

    $result = db_query('SELECT nid FROM {feeds_node_item} WHERE feed_nid = %d', $source->feed_nid);
    while ($node = db_fetch_object($result)) {
      node_delete($node->nid);
      $deleted++;
    }

    // Set message.
    if ($deleted) {
      drupal_set_message(t('Deleted !number nodes.', array('!number' => $deleted)));
    }
    else {
      drupal_set_message(t('There is no content to be deleted.'));
    }
  }

  /**
   * Return available mapping targets.
   */
  public function getMappingTargets() {
    $targets = array(
      'title' => array(
        'name' => t('Title'), // @todo: write out.
       ),
      'status' => array(
        'name' => t('Published status'),
      ),
      'created' => array(
        'name' => t('Published date'),
      ),
      'teaser' => array(
        'name' => t('Body'),
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
    // Assign an aggregated node always to anonymous.
    $target_node->uid = 0;

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
    elseif (in_array($target_element, array('title', 'teaser', 'status', 'created'))) {
      $target_node->$target_element = $value;
    }
  }

  /**
   * Get nid of an existing feed item node if available.
   */
  public function existingItemId($source_item, FeedsSource $source) {
    $parser = feeds_feed($this->id)->parser;
    $targets = array();
    foreach ($this->config['mappings'] as $mapping) {
      if ($mapping['unique']) {
        $targets[$mapping['target']] = $parser->getSourceElement($source_item, $mapping['source']);
      }
    }
    foreach ($targets as $target => $value) {
      switch ($target) {
        case 'url':
          $nid = db_result(db_query('SELECT nid FROM {feeds_node_item} WHERE feed_nid = %d AND url = "%s"', $source->feed_nid, $value));
          break;
        case 'guid':
          $nid = db_result(db_query('SELECT nid FROM {feeds_node_item} WHERE feed_nid = %d AND guid = "%s"', $source->feed_nid, $value));
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
    $type = isset($types['story']) ? 'story' : key($types);
    return array(
      'content_type' => $type, // @todo: provide default content type feed_item.
      'update_existing' => 0,
      'mappings' => array(
        '0' => array(
          'source' => 'title',
          'target' => 'title',
          'unique' => FALSE,
        ),
        '1' => array(
          'source' => 'description',
          'target' => 'teaser',
          'unique' => FALSE,
        ),
        '2' => array(
          'source' => 'timestamp',
          'target' => 'created',
          'unique' => FALSE,
        ),
        '3' => array(
          'source' => 'url',
          'target' => 'url',
          'unique' => TRUE,
        ),
        '4' => array(
          'source' => 'guid',
          'target' => 'guid',
          'unique' => TRUE,
        ),
      ),
    );
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