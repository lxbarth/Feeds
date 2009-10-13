<?php
// $Id$

/**
 * @file
 * Views handler for showing a feeds source config field.
 */

class feeds_views_handler_field_source extends views_handler_field {

  /**
   * Override parent::render().
   */
  function render($values) {
    $value = unserialize($values->{$this->field_alias});
    if (isset($value['FeedsHttpFetcher']['url'])) {
      return check_url($value['FeedsHttpFetcher']['url']);
    }
    elseif (isset($value['FeedsFileFetcher']['feeds_source'])) {
      // @todo: this is untested.
      return $GLOBALS['base_url'] .'/'. check_plain($value['FeedsFileFetcher']['feeds_source']);
    }
    return '';
  }

  /**
   * Disallow advanced rendering.
   */
  function allow_advanced_render() {
    return FALSE;
  }
}