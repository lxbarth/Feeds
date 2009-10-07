<?php
// $Id$

/**
 * @file
 * FeedsImporter class and related.
 */

/**
 * Implementation of IFeedsImporter.
 */
class FeedsImporter {

  /**
   * Create a single instance of FeedsImporter.
   */
  public static function instance() {
    static $instance;
    if (!isset($instance)) {
      $class = variable_get('feeds_importer_class', 'FeedsImporter');
      $instance = new $class();
    }
    return $instance;
  }

  /**
   * Protect constructor.
   */
  protected function __construct() {}

  /**
   * Import a feed: execute, fetching, parsing and processing stage.
   *
   * @throws Exception
   *   If a problem with fetching, parsing or processing occured.
   *   @todo: Iron out and document potential Exceptions.
   *   @todo: Support batching.
   */
  public function import(Feed $feed, FeedsSource $source) {
    $result = $feed->fetcher->fetch($source);
    $result = $feed->parser->parse($result, $source);
    $feed->processor->process($result, $source);
  }

  /**
   * Remove all Items from a feed.
   *
   * @todo: Run purge on fetcher parser and processor.
   */
  public function purge(Feed $feed, FeedsSource $source) {
    $feed->processor->purge($source);
  }
}