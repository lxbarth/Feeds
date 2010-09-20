<?php
// $Id: FeedsOPMLParser.inc,v 1.7 2010/09/17 17:25:19 alexb Exp $

/**
 * @file
 * OPML Parser plugin.
 */

/**
 * Feeds parser plugin that parses OPML feeds.
 */
class FeedsOPMLParser extends FeedsParser {

  /**
   * Implements FeedsParser::parse().
   */
  public function parse(FeedsImportBatch $batch, FeedsSource $source) {
    feeds_include_library('opml_parser.inc', 'opml_parser');
    $result = opml_parser_parse($batch->getRaw());
    $batch->setTitle($result['title']);
    $batch->setItems($result['items']);
  }

  /**
   * Return mapping sources.
   */
  public function getMappingSources() {
    return array(
      'title' => array(
        'name' => t('Feed title'),
        'description' => t('Title of the feed.'),
      ),
      'xmlurl' => array(
        'name' => t('Feed URL'),
        'description' => t('URL of the feed.'),
      ),
    ) + parent::getMappingSources();
  }
}
