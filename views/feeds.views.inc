<?php
// $Id$

/**
 * @file
 * Views integration for Feeds tables.
 *
 * @see http://drupal.org/project/views
 */

/**
 * Implementation of hook_views_data().
 */
function feeds_views_data() {
  $data = array();

  $data['feeds_source']['table'] = array(
    'group' => 'Feeds source',
    'base' => array(
      'field' => array('feed_nid', 'id'),
      'title' => 'Feeds source',
      'help' => 'The source information for a feed, for example its URL',
    ),
  );
  $data['feeds_source']['feed_nid'] = array(
    'title' => 'Feed node id',
    'help' => 'Contains the node id of a feed node if the feed\'s configuration is attached to a content type. Otherwise contains 0.',
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
      'allow empty' => TRUE,
      'help' => 'Filter on a Feeds Source\'s feed_nid field.',
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_mumeric',
      'help' => 'Argument on a Feeds Source\'s feed_nid field.',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
      'help' => 'Sort Feeds Source\'s feed_nid field.',
    ),
  );

  return $data;
}