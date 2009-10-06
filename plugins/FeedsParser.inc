<?php
// $Id$

/**
 * Defines the object a Parser returns on parser().
 */
class FeedsParserResult extends FeedsResult {
  // Define valid types.
  protected $validTypes = array('simple', 'syndication');

  /**
   * Override constructor to define a default type.
   */
  public function __construct($value, $type = 'simple') {
    parent::__construct($value, $type);
  }
}

/**
 * Abstract class, defines interface for parsers.
 */
abstract class FeedsParser extends FeedsConfigurable {

  /**
   * Parse content fetched by fetcher.
   *
   * Stub method. Extending classes must implement this method.
   *
   * @param $fetcherResult
   *   Content returned by fetcher.
   * @param $feed_nid
   *   If this configuration is attached to a content type, $feed_nid contains the
   *   node id of the Feed node.
   * @return
   *   A parsed array.
   *   @todo: define this array (object?).
   */
  public abstract function parse(FeedsFetcherResult $fetcherResult, $feed_nid = NULL);

  /**
   * Declare the possible mapping sources that this parser produces.
   *
   * @return
   *   An array of mapping sources, or FALSE if the sources can be defined by typing a value
   *   in a text field.
   *
   *   Example:
   *   array(
   *     'title' => t('Title'),
   *     'created' => t('Published date'),
   *     'url' => t('Feed item URL'),
   *     'guid' => t('Feed item GUID'),
   *   )
   */
  public function getMappingSources() {
    return FALSE;
  }

  /**
   * Get an element identified by $element_key of the given item.
   * The element key corresponds to the values in the array returned by
   * FeedsParser::getMappingSources().
   */
  public function getSourceElement($item, $element_key) {
    return $item[$element_key];
  }

  /**
   * Save changes to the configuration of this object.
   * Delegate saving to parent Feed which will collect
   * information from this object and store it.
   */
  public function save() {
    $feed = feeds_feed_instance($this->id);
    $feed->save();
  }
}