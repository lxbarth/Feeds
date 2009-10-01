<?php
// $Id$
/**
 * @file
 * Menu callbacks, form callbacks and helpers.
 */

/**
 * Render a page of available feed configuration.
 */
function feeds_page() {
    $rows = array();
  if ($feeds = feeds_load_all()) {
    foreach ($feeds as $feed) {
      if (empty($feed->config['content_type'])) {
        $link = 'import/'. $feed->id;
        $title = $feed->config['name'];
      }
      elseif (user_access('create '. $feed->config['content_type'] .' content')) {
        $link = 'node/add/'. $feed->config['content_type'];
        $title = t('!config (attached to !type)', array('!config' => $feed->config['name'], '!type' => $feed->config['content_type']));
      }
      $rows[] = array(
        l($title, $link),
        '@todo: description.',
      );
    }
  }
  $header = array(
    t('Configuration'),
    t('Description'),
  );
  return theme('table', $header, $rows);
}