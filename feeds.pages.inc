<?php
// $Id$
/**
 * @file
 * Menu callbacks, form callbacks and helpers.
 */

/**
 * Render a feeds import form.
 */
function feeds_form(&$form_state, $feed_id) {
  $feed = feeds_instance('Feed', $feed_id);
  $form = $feed->fetcher->sourceForm($form_state);
  $form['#feed'] = $feed;
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import'),
  );
  return $form;
}

/**
 * Validation handler for feeds_form_submit().
 */
function feeds_form_validate($form, &$form_state) {
  $form['#feed']->fetcher->sourceFormValidate($form, &$form_state);
}

/**
 * Submit handler for feeds_form_submit().
 */
function feeds_form_submit($form, &$form_state) {
  $form['#feed']->fetcher->sourceFormSubmit($form, &$form_state);
}

/**
 * Render an overview page over available feeds configurations.
 */
function feeds_page() {
    $rows = array();
  if ($feeds = feeds_load_all()) {
    foreach ($feeds as $feed) {
      if (empty($feed->config['content_type'])) {
        $link = 'import/'. $feed->id;
        $title = $feed->config['name'];
      }
      else {
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