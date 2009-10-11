<?php
// $Id$

/**
 * Defines the object a Fetcher returns on fetch().
 */
class FeedsFetcherResult extends FeedsResult {
  // Define valid types.
  // @todo: does text/filepath make sense?
  // @todo: If convenient, we could expand on this concept and build content
  //   type negotiation between Fetchers and Parsers.
  protected $validTypes = array('text/filepath', 'text/xml');
}

/**
 * Abstract class, defines shared functionality between fetchers.
 *
 * Implements FeedsSourceInfoInterface to expose source forms to Feeds.
 */
abstract class FeedsFetcher extends FeedsPlugin {

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
  public abstract function fetch(FeedsSource $source);
}
