<?php
// $Id: FeedsSyndicationParser.inc,v 1.3 2009/09/19 15:52:46 alexb Exp $

/**
 * Class definition for Common Syndication Parser.
 * 
 * Parses RSS and Atom feeds.
 */
class FeedsSyndicationParser extends FeedsParser {

  /**
   * Parses a raw string and returns a Feed object from it.
   */
  public function parse(FeedsFetcherResult $fetcherResult, $nid = NULL) {
    if ($fetcherResult->type == 'text/filepath') {
      $string = file_get_contents($fetcherResult->value);
    }
    else {
      $string = $fetcherResult->value;
    }
    feeds_include('common_syndication_parser', 'libraries');
    dsm(common_syndication_parser_parse($string));
  }

  /**
   * Return mapping sources.
   */
  public function getMappingSources() {
    return array(
      'title' => t('Title'), 
       'description' => t('Description'), 
       'link' => t('Item URL (link)'), 
       'guid' => t('Item GUID'), 
       'tags' => t('Categories'),
     );
  }
}