<?php
// $Id$

/**
 * Abstract class, defines interface for parsers.
 */
class FeedsParser extends FeedsConfigurable {

  /**
   * Parse content fetched by fetcher.
   * 
   * Stub method. Extending classes must implement this method.
   *
   * @param $raw
   *   Content returned by fetcher.
   * @return
   *   A parsed array. 
   *   @todo: define this array (object?).
   */
  public function parse($raw) {
    return NULL;
  }

  /**
   * Declare the possible mapping sources that this parser produces.
   * 
   * @return 
   *   An array of mapping sources.
   */
  public function getMappingSources() {
    return NULL;
  }
}