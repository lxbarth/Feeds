<?php
// $Id: FeedsNodeProcessor.inc,v 1.69.2.4 2010/10/01 17:45:18 alexb Exp $

/**
 * @file
 * Class definition of FeedsNodeProcessor.
 */

// Deprecated. Use FEEDS_SKIPE_EXISTING, FEEDS_REPLACE_EXISTNG,
// FEEDS_UPDATE_EXISTING instead.
define('FEEDS_NODE_SKIP_EXISTING', 0);
define('FEEDS_NODE_REPLACE_EXISTING', 1);
define('FEEDS_NODE_UPDATE_EXISTING', 2);

// "Use the present default format"
define('FEEDS_NODE_DEFAULT_FORMAT', -1);

/**
 * Creates nodes from feed items.
 */
class FeedsNodeProcessor extends FeedsProcessor {
  /**
   * Implements FeedsProcessor::process().
   */
  public function process(FeedsSource $source, FeedsParserResult $parser_result) {
    $state = $source->state(FEEDS_PROCESS);

    while ($item = $parser_result->shiftItem()) {
      if (!($nid = $this->existingItemId($source, $parser_result)) ||
           ($this->config['update_existing'] != FEEDS_SKIP_EXISTING)) {

        // Only proceed if item has actually changed.
        $hash = $this->hash($item);
        if (!empty($nid) && $hash == $this->getHash($nid)) {
          continue;
        }

        // Assemble node, map item to it, save.
        try {
          $node = $this->buildNode($nid, $source->feed_nid);
          $node->feeds_node_item->hash = $hash;
          $this->map($source, $parser_result, $node);
          node_save($node);
          if (!empty($nid)) {
            $state->updated++;
          }
          else {
            $state->created++;
          }
        }
        catch (Exception $e) {
          drupal_set_message($e->getMessage(), 'warning');
          watchdog('feeds', $e->getMessage(), array(), WATCHDOG_WARNING);
        }
      }
    }

    // Set messages if we're done.
    if ($source->progressImporting() != FEEDS_BATCH_COMPLETE) {
      return;
    }
    if ($state->created) {
      drupal_set_message(format_plural($state->created, 'Created @number @type node.', 'Created @number @type nodes.', array('@number' => $state->created, '@type' => node_type_get_name($this->config['content_type']))));
    }
    elseif ($state->updated) {
      drupal_set_message(format_plural($state->updated, 'Updated @number @type node.', 'Updated @number @type nodes.', array('@number' => $state->updated, '@type' => node_type_get_name($this->config['content_type']))));
    }
    else {
      drupal_set_message(t('There is no new content.'));
    }
  }

  /**
   * Implements FeedsProcessor::clear().
   */
  public function clear(FeedsSource $source) {
    $state = $source->state(FEEDS_PROCESS_CLEAR);
    if (empty($state->total)) {
      $state->total = db_query("SELECT COUNT(n.nid) FROM {node} n JOIN {feeds_node_item} fn ON n.nid = fn.nid WHERE fn.id = :id AND fn.feed_nid = :nid", array(':id' => $source->id, ':nid' => $source->feed_nid))->fetchField();
    }
    $count = $this->getLimit();
    $nids = array();
    $nodes = db_query_range("SELECT n.nid FROM {node} n JOIN {feeds_node_item} fn ON n.nid = fn.nid WHERE fn.id = :id AND fn.feed_nid = :nid", 0, $count, array(':id' => $source->id, ':nid' => $source->feed_nid));
    foreach ($nodes as $node) {
      $nids[$node->nid] = $node->nid;
      $state->deleted++;
    }
    node_delete_multiple($nids);
    if (db_query_range("SELECT 1 FROM {node} n JOIN {feeds_node_item} fn ON n.nid = fn.nid WHERE fn.id = :id AND fn.feed_nid = :nid", 0, 1, array(':id' => $source->id, ':nid' => $source->feed_nid))->fetchField()) {
      $state->progress($state->total, $state->deleted);
      return;
    }

    // Set message.
    drupal_get_messages('status');
    if ($state->deleted) {
      drupal_set_message(format_plural($state->deleted, 'Deleted @number node.', 'Deleted @number nodes.', array('@number' => $state->deleted)));
    }
    else {
      drupal_set_message(t('There is no content to be deleted.'));
    }
    // Make sure we are finished, there may be a discrepancy between total and
    // deleted if other processes have deleted nodes in the meantime.
    $state->progress = FEEDS_BATCH_COMPLETE;
  }

  /**
   * Report processing and clearing limit.
   */
  public function getLimit() {
    return variable_get('feeds_process_limit', FEEDS_PROCESS_LIMIT);
  }

  /**
   * Implement expire().
   */
  public function expire($time = NULL) {
    if ($time === NULL) {
      $time = $this->expiryTime();
    }
    if ($time == FEEDS_EXPIRE_NEVER) {
      return;
    }
    $count = $this->getLimit();
    $nodes = db_query_range("SELECT n.nid FROM {node} n JOIN {feeds_node_item} fni ON n.nid = fni.nid WHERE fni.id = :id AND n.created < :created", 0, $count, array(':id' => $this->id, ':created' => REQUEST_TIME - $time));
    foreach ($nodes as $node) {
      _feeds_node_delete($node->nid);
    }
    if (db_query_range("SELECT 1 FROM {node} n JOIN {feeds_node_item} fni ON n.nid = fni.nid WHERE fni.id = :id AND n.created < :created", 0, 1, array(':id' => $this->id, ':created' => REQUEST_TIME - $time))->fetchField()) {
      return FEEDS_BATCH_ACTIVE;
    }
    return FEEDS_BATCH_COMPLETE;
  }

  /**
   * Return expiry time.
   */
  public function expiryTime() {
    return $this->config['expire'];
  }

  /**
   * Override parent::configDefaults().
   */
  public function configDefaults() {
    $types = node_type_get_names();
    $type = isset($types['article']) ? 'article' : key($types);
    return array(
      'content_type' => $type,
      'input_format' => FEEDS_NODE_DEFAULT_FORMAT,
      'update_existing' => FEEDS_SKIP_EXISTING,
      'expire' => FEEDS_EXPIRE_NEVER,
      'mappings' => array(),
      'author' => 0,
    );
  }

  /**
   * Override parent::configForm().
   */
  public function configForm(&$form_state) {
    $types = node_type_get_names();
    array_walk($types, 'check_plain');
    $form = array();
    $form['content_type'] = array(
      '#type' => 'select',
      '#title' => t('Content type'),
      '#description' => t('Select the content type for the nodes to be created. <strong>Note:</strong> Users with "import !feed_id feeds" permissions will be able to <strong>import</strong> nodes of the content type selected here regardless of the node level permissions. Further, users with "clear !feed_id permissions" will be able to <strong>delete</strong> imported nodes regardless of their node level permissions.', array('!feed_id' => $this->id)),
      '#options' => $types,
      '#default_value' => $this->config['content_type'],
    );
    $format_options = array(FEEDS_NODE_DEFAULT_FORMAT => t('Default format'));
    global $user;
    $formats = filter_formats($user);
      foreach ($formats as $format) {
        $format_options[$format->format] = check_plain($format->name);
      }
    $form['input_format'] = array(
      '#type' => 'select',
      '#title' => t('Input format'),
      '#description' => t('Select the input format for the body field of the nodes to be created.'),
      '#options' => $format_options,
      '#default_value' => $this->config['input_format'],
    );
    $author = user_load($this->config['author']);
    $form['author'] = array(
      '#type' => 'textfield',
      '#title' => t('Author'),
      '#description' => t('Select the author of the nodes to be created - leave empty to assign "anonymous".'),
      '#autocomplete_path' => 'user/autocomplete',
      '#default_value' => empty($author->name) ?  'anonymous' : check_plain($author->name),
    );
    $period = drupal_map_assoc(array(FEEDS_EXPIRE_NEVER, 3600, 10800, 21600, 43200, 86400, 259200, 604800, 604800 * 4, 604800 * 12, 604800 * 24, 31536000), 'feeds_format_expire');
    $form['expire'] = array(
      '#type' => 'select',
      '#title' => t('Expire nodes'),
      '#options' => $period,
      '#description' => t('Select after how much time nodes should be deleted. The node\'s published date will be used for determining the node\'s age, see Mapping settings.'),
      '#default_value' => $this->config['expire'],
    );
    $form['update_existing'] = array(
      '#type' => 'radios',
      '#title' => t('Update existing nodes'),
      '#description' => t('Select how existing nodes should be updated. Existing nodes will be determined using mappings that are a "unique target".'),
      '#options' => array(
        FEEDS_SKIP_EXISTING => 'Do not update existing nodes',
        FEEDS_REPLACE_EXISTING => 'Replace existing nodes',
        FEEDS_UPDATE_EXISTING => 'Update existing nodes (slower than replacing them)',
      ),
      '#default_value' => $this->config['update_existing'],
    );
    return $form;
  }

  /**
   * Override parent::configFormValidate().
   */
  public function configFormValidate(&$values) {
    if ($author = user_load_by_name($values['author'])) {
      $values['author'] = $author->uid;
    }
    else {
      $values['author'] = 0;
    }
  }

  /**
   * Reschedule if expiry time changes.
   */
  public function configFormSubmit(&$values) {
    if ($this->config['expire'] != $values['expire']) {
      feeds_reschedule($this->id);
    }
    parent::configFormSubmit($values);
  }

  /**
   * Override setTargetElement to operate on a target item that is a node.
   */
  public function setTargetElement($target_node, $target_element, $value) {
    switch ($target_element) {
      case 'url':
      case 'guid':
        $target_node->feeds_node_item->$target_element = $value;
        break;
      case 'title':
      case 'status':
      case 'nid':
      case 'uid':
        $target_node->$target_element = $value;
        break;
      case 'created':
        $target_node->created = REQUEST_TIME;
        if (is_numeric($value)) {
          $target_node->created = $value;
        }
        elseif (is_string($value)) {
          $value = new FeedsDateTimeElement($value);
          $value->created = $value->getValue();
        }
        elseif ($value instanceof FeedsDateTimeElement) {
          $target_node->created = $value->getValue();
        }
        break;
    }
  }

  /**
   * Return available mapping targets.
   */
  public function getMappingTargets() {
    $type = node_type_get_type($this->config['content_type']);
    $targets = array();
    if ($type->has_title) {
      $targets['title'] = array(
        'name' => t('Title'),
        'description' => t('The title of the node.'),
      );
    }
    $targets += array(
      'nid' => array(
        'name' => t('Node ID'),
        'description' => t('The nid of the node. NOTE: use this feature with care, node ids are usually assigned by Drupal.'),
        'optional_unique' => TRUE,
      ),
      'uid' => array(
        'name' => t('User ID'),
        'description' => t('The Drupal user ID of the node author.'),
      ),
      'status' => array(
        'name' => t('Published status'),
        'description' => t('Whether a node is published or not. 1 stands for published, 0 for not published.'),
      ),
      'created' => array(
        'name' => t('Published date'),
        'description' => t('The UNIX time when a node has been published.'),
      ),
      'url' => array(
        'name' => t('URL'),
        'description' => t('The external URL of the node. E. g. the feed item URL in the case of a syndication feed. May be unique.'),
        'optional_unique' => TRUE,
      ),
      'guid' => array(
        'name' => t('GUID'),
        'description' => t('The external GUID of the node. E. g. the feed item GUID in the case of a syndication feed. May be unique.'),
        'optional_unique' => TRUE,
      ),
    );

    // Let other modules expose mapping targets.
    self::loadMappers();
    feeds_alter('feeds_processor_targets', $targets, 'node', $this->config['content_type']);

    return $targets;
  }

  /**
   * Get nid of an existing feed item node if available.
   */
  protected function existingItemId(FeedsSource $source, FeedsParserResult $result) {

    // Iterate through all unique targets and test whether they do already
    // exist in the database.
    foreach ($this->uniqueTargets($source, $result) as $target => $value) {
      switch ($target) {
        case 'nid':
          $nid = db_query("SELECT nid FROM {node} WHERE nid = :nid", array(':nid' => $value))->fetchField();
          break;
        case 'url':
          $nid = db_query("SELECT nid FROM {feeds_node_item} WHERE feed_nid = :nid AND id = :id AND url = :url", array(':nid' => $source->feed_nid, ':id' => $source->id, ':url' => $value))->fetchField();
          break;
        case 'guid':
          $nid = db_query("SELECT nid FROM {feeds_node_item} WHERE feed_nid = :nid AND id = :id AND guid = :guid", array(':nid' => $source->feed_nid, ':id' => $source->id, ':guid' => $value))->fetchField();
          break;
      }
      if ($nid) {
        // Return with the first nid found.
        return $nid;
      }
    }
    return 0;
  }

  /**
   * Creates a new node object in memory and returns it.
   */
  protected function buildNode($nid, $feed_nid) {
    $node = new stdClass();
    $populate = FALSE;
    if (empty($nid)) {
      $node->created = REQUEST_TIME;
      $populate = TRUE;
    }
    else {
      if ($this->config['update_existing'] == FEEDS_UPDATE_EXISTING) {
        $node = node_load($nid, NULL, TRUE);
      }
      else {
        $node = db_query("SELECT nid, vid, created FROM {node} WHERE nid = :nid", array(':nid' => $nid))->fetch();
        $populate = TRUE;
      }
    }
    if ($populate) {
      $node->type = $this->config['content_type'];
      $node->changed = REQUEST_TIME;
      $node->format = ($this->config['input_format'] == FEEDS_NODE_DEFAULT_FORMAT) ? filter_fallback_format() : $this->config['input_format'];
      $node->feeds_node_item = new stdClass();
      $node->feeds_node_item->id = $this->id;
      $node->feeds_node_item->imported = REQUEST_TIME;
      $node->feeds_node_item->feed_nid = $feed_nid;
      $node->feeds_node_item->url = '';
      $node->feeds_node_item->guid = '';
    }

    // Give mappers a hint at what they're operating on.
    $node->entity_type = 'node';

    // Let other modules populate default values.
    node_object_prepare($node);

    // Populate properties that are set by node_object_prepare().
    $node->log = 'Created/updated by FeedsNodeProcessor';
    if ($populate) {
      $node->uid = $this->config['author'];
    }
    return $node;
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
  protected function getHash($nid) {
    if ($hash = db_query("SELECT hash FROM {feeds_node_item} WHERE nid = :nid", array(':nid' => $nid))->fetchField()) {
      // Return with the hash.
      return $hash;
    }
    return '';
  }
}
