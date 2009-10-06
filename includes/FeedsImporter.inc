<?php
// $Id$

/**
 * @file
 * FeedsImporter class and related.
 */

interface IFeedsImporter {
  /**
   * Import FeedsSource with Feed.
   */
  public function import(Feed $feed, FeedsSource $source);
  /**
   * Remove all items for a FeedsSource of Feed configuration.
   */
  public function purge(Feed $feed, FeedsSource $source);
}

/**
 * Implementation of IFeedsImporter.
 */
class FeedsImporter implements IFeedsImporter {

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
   * Implementation of IFeedsImporter::import().
   *
   * @throws Exception
   *   If a problem with fetching, parsing or processing occured.
   */
  public function import(Feed $feed, FeedsSource $source) {
    $result = $feed->fetcher->fetch($source);
    $result = $feed->parser->parse($result, $source);
    $feed->processor->process($result, $source);
  }

  /**
   * Implementation of IFeedsImporter::purge().
   */
  public function purge(Feed $feed, FeedsSource $source) {
    $feed->processor->purge($source);
  }
}