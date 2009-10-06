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
  public function process(FeedsParserResult $parserResult, FeedsSource $source) {
    return NULL;
  }
  public function purge(FeedsSource $source) {

  }
}