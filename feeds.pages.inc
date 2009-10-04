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

/**
 * Render a feeds import form on node/id/import pages.
 */
function feeds_import_node_form(&$form_state, $node) {
  $feed = feeds_get_by_content_type($node->type);

  $form = array();
  $form['#feed_nid'] = $node->nid;
  $form['#feed_id'] = $feed->id;
  return confirm_form($form, t('Import all content from feed?'), 'node/'. $node->nid, '', t('Import'), t('Cancel'), 'confirm feeds update');
}

/**
 * Submit handler for feeds_import_node_form().
 */
function feeds_import_node_form_submit($form, $form_state) {
  $feed = feeds_instance('Feed', $form['#feed_id']);
  $feed->import($form['#feed_nid']);
}

/**
 * Render a feeds import form on import/[config] pages.
 */
function feeds_import_form(&$form_state, $feed_id) {
  $feed = feeds_instance('Feed', $feed_id);

  $form = array();
  $form['#feed_id'] = $feed->id;
  // @todo: Move this into fetcher?
  $form['#attributes']['enctype'] = 'multipart/form-data';
  // @todo: let fetchers build this fieldset, allows for tweaking the title and adding description.
  $form['feeds'] = array(
    '#type' => 'fieldset',
    '#title' => t('Feed'),
    '#tree' => TRUE,
  );
  $form['feeds'] += $feed->fetcher->sourceForm($form['nid']['#value']);
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import'),
  );
  return $form;
}

/**
 * Render a feeds delete form.
 *
 * Used on both node pages and configuration pages.
 * Therefore either $feed_id or $node may be missing.
 */
function feeds_delete_items_form(&$form_state, $feed_id, $node = NULL) {
  if (empty($node)) {
    $feed = feeds_instance('Feed', $feed_id);
  }
  else {
    $feed = feeds_get_by_content_type($node->type);
    $form['#feed_nid'] = $node->nid;
  }
  // Form cannot pass on feed object.
  $form['#feed_id'] = $feed->id;
  return confirm_form($form, t('Delete all content from feed?'), 'node/'. $node->nid, '', t('Delete'), t('Cancel'), 'confirm feeds update');
}

/**
 * Submit handler for feeds_delete_items_form().
 */
function feeds_delete_items_form_submit($form, &$form_state) {
  $feed = feeds_instance('Feed', $form['#feed_id']);
  $feed->purge($form['#feed_nid']);
}

/**
 * Render a control box.
 */
function feeds_control_box($feed, $nid = NULL) {
  if (user_access('administer feeds')) {
    $rows = array();
    $rows[] = array(
      l(t('Reimport feed'), 'node/2/refresh'),
      // @todo adjust description by configuration.
      t('Reads source information, creates items that do not exist and optionally updates existing items.'),
    );
    $rows[] = array(
      l(t('Delete all items'), 'node/2/delete-items'),
      t('Deletes all content imported by that feed.'),
    );
    $element = array(
      '#title' => t('Feed options'),
      '#value' => theme('table', array(t('Option'), t('Description')), $rows),
    );
    return theme('fieldset', $element);
  }
}