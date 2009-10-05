<?php
// $Id$
/**
 * @file
 * Definition of FeedsDataProcessor.
 */

/**
 * @todo: Implement.
 * @todo: Find way of showing this processor, but marking 'inactive' if Data module is not present.
 */
class FeedsDataProcessor extends FeedsProcessor {
  public function process(FeedsParserResult $parserResult, $feed_nid = NULL) {
    return NULL;
  }
  public function purge($feed_nid = NULL) {

  }
}