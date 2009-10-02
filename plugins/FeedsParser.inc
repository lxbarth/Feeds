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
   * @param $nid
   *   If this configuration is attached to a content type, $nid contains the
   *   node id of the Feed node.
   * @return
   *   A parsed array. 
   *   @todo: define this array (object?).
   */
  public abstract function parse(FeedsFetcherResult $fetcherResult, $nid = NULL);

  /**
   * Declare the possible mapping sources that this parser produces.
   * 
   * @return 
   *   An array of mapping sources.
   */
  public abstract function getMappingSources();
}