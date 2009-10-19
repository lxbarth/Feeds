<?php
// $Id$

/**
 * @file
 * OPML Parser plugin.
 */

/**
 * Feeds parser plugin that parses OPML feeds.
 */
class FeedsOPMLParser extends FeedsParser {

  /**
   * Parses a raw string and returns a Feed object from it.
   */
  public function parse(FeedsFetcherResult $fetcherResult, FeedsSource $source) {
    if ($fetcherResult->type == 'text/filepath') {
      $string = file_get_contents($fetcherResult->value);
    }
    else {
      $string = $fetcherResult->value;
    }
    feeds_include_library('opml_parser.inc', 'opml_parser');
    return new FeedsParserResult(opml_parser_parse($string), 'syndication');
  }

  /**
   * Return mapping sources.
   */
  public function getMappingSources() {
    return array(
      'title' => t('Feed title'),
      'xmlurl' => t('Feed URL'),
     );
  }
}