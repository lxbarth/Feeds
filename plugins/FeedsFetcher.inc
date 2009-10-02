<?php
// $Id$

/**
 * Abstract class, defines interface for fetchers.
 * 
 * Not using interfaces because we need a simple inheritence tree for determining the 
 * plugin type. See hook_feeds_plugin().
 * 
 * @todo: There is a potential conceptual problem:
 * 
 * - We assume that only a feed source can be stored per feed and ***no other per-feed 
 *   configuration information***
 * - This might lead to ugly hacks when other plugins in fact need a per node configuration.
 * - Possible solution: Abstract sourceLoad() etc. and push into FeedsConfigurable, or make 
 *   FeedsConfigurable cascadable.
 */
abstract class FeedsFetcher extends FeedsConfigurable {

  /**
   * Callback methods, exposes source form.
   */
  public function sourceForm(&$form_state, $nid = NULL) {
    if (!$source = $this->loadSource($nid)) {
      $source = $this->config['source'];
    }
    $form = array();
    $form['feeds_source'] = array(
      '#type' => 'textfield',
      '#title' => t('URL'),
      '#description' => t('Enter the URL for this feed.'),
      '#default_value' => $source,
    );
    return $form;
  }

  /**
   * Validation handler for sourceForm().
   */
  public function sourceFormValidate($form, &$values) {
    if (valid_url($values[''])) {
      
    }
  }

  /**
   * Load source.
   */
  public function loadSource($nid = NULL) {
    if (empty($nid)) {
      $result = db_query('SELECT * FROM {feeds_source} WHERE id = "%s" AND class = "%s"', $this->id, get_class($this));
    }
    else {
      $result = db_query('SELECT * FROM {feeds_source} WHERE id = "%s" AND class = "%s" AND nid = %d', $this->id, get_class($this), $nid);
    }
    if ($source = db_fetch_object($result)) {
      return unserialize($source->source);
    }
    return FALSE;
  }

  /**
   * Save a source.
   * 
   * @param $source
   *   Source array in the convention of this fetcher.
   * 
   * @param $nid
   *   If configuration is attached to a content type, pass in the feed node's 
   *   node id.
   */
  public function saveSource($source, $nid = NULL) {
    $this->deleteSource($nid);
    $object = array(
      'id' => $this->id,
      'nid' => $nid,
      'class' => get_class($this),
      'source' => $source['feeds_source'], // This is the same key that we defined in sourceForm().
    );
    drupal_write_record('feeds_source', $object);
  }

  /**
   * Delete a source.
   */
  public function deleteSource($nid = NULL) {
    if (empty($nid)) {
      db_query('DELETE FROM {feeds_source} WHERE id = "%s" AND class = "%s"', $this->id, get_class($this));
    }
    else {
      db_query('DELETE FROM {feeds_source} WHERE id = "%s" AND class = "%s" AND nid = %d', $this->id, get_class($this), $nid);
    }
  }
  
  /**
   * Fetch content from a source and return it.
   * 
   * Stub method. Every class that extends FeedsFetcher must implement this method.
   *
   * @param $source 
   *   Source value as entered by user through sourceForm().
   * 
   * @todo: Define format of return value?
   */
  public abstract function fetch($nid = NULL);
}
