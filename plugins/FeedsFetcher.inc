<?php
// $Id$

/**
 * Defines the object a Fetcher returns on fetch().
 */
class FeedsFetcherResult extends FeedsResult {
  // Define valid types.
  // @todo: does text/filepath make sense?
  // @todo: If convenient, we could expand on this concept and build content type negotiation
  // between Fetchers and Parsers.
  protected $validTypes = array('text/filepath', 'text/xml');
}

/**
 * This exception gets thrown when no source is available
 * for a given feed_nid
 */
class FeedsNoSourceException extends Exception {
}

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
   *
   * @todo: move HTTP fetcher specific code down into HTTP fetcher.
   */
  public function sourceForm($feed_nid = NULL) {
    if (!$source = $this->loadSource($feed_nid)) {
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
   *
   * @param $form
   *   The form to perform the validation on.
   * @param $values
   *   The values to perform the validation on.
   *   This is a subsection of the usual $form_state['values'] that
   *   only contains the values from the form defined in sourceForm().
   */
  public function sourceFormValidate($form, &$values) {
  }

  /**
   * Load source.
   */
  public function loadSource($feed_nid = NULL) {
    if (empty($feed_nid)) {
      $result = db_query('SELECT * FROM {feeds_source} WHERE id = "%s" AND class = "%s" AND feed_nid = 0', $this->id, get_class($this));
    }
    else {
      $result = db_query('SELECT * FROM {feeds_source} WHERE id = "%s" AND class = "%s" AND feed_nid = %d', $this->id, get_class($this), $feed_nid);
    }
    if ($source = db_fetch_object($result)) {
      return unserialize($source->source);
    }
    throw new FeedsNoSourceException('No source found');
  }

  /**
   * Save a source.
   *
   * @param $source
   *   Source array in the convention of this fetcher.
   *
   * @param $feed_nid
   *   If configuration is attached to a content type, pass in the feed node's
   *   node id.
   */
  public function saveSource($source, $feed_nid = NULL) {
    $object = array(
      'id' => $this->id,
      'feed_nid' => $feed_nid,
      'class' => get_class($this),
      'source' => $source['feeds_source'], // This is the same key that we defined in sourceForm().
    );
    // Make sure a source record is present at all time, try to update first, then insert.
    if (SAVED_UPDATED != drupal_write_record('feeds_source', $object, array('id', 'feed_nid'))) {
      drupal_write_record('feeds_source', $object);
    }
  }

  /**
   * Delete a source.
   */
  public function deleteSource($feed_nid = NULL) {
    if (empty($feed_nid)) {
      db_query('DELETE FROM {feeds_source} WHERE id = "%s" AND class = "%s"', $this->id, get_class($this));
    }
    else {
      db_query('DELETE FROM {feeds_source} WHERE id = "%s" AND class = "%s" AND feed_nid = %d', $this->id, get_class($this), $feed_nid);
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
  public abstract function fetch($feed_nid = NULL);

  /**
   * Save changes to the configuration of this object.
   * Delegate saving to parent Feed which will collect
   * information from this object and store it.
   */
  public function save() {
    $feed = feeds_instance('Feed', $this->id);
    $feed->save();
  }
}
