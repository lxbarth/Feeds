<?php

/**
 * Implementation of hook_views_default_views().
 */
function feeds_news_views_default_views() {
  $views = array();

  // Exported view: feeds_defaults_feed_items
  $view = new view;
  $view->name = 'feeds_defaults_feed_items';
  $view->description = 'Show feed items for a feed node. Use together with default importer configuration "Feed".';
  $view->tag = 'Feeds defaults';
  $view->view_php = '';
  $view->base_table = 'node';
  $view->is_cacheable = FALSE;
  $view->api_version = '3.0-alpha1';
  $view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */
  
/* Display: Defaults */
  $handler = $view->new_display('default', 'Defaults', 'default');
  $handler->display->display_options['access']['type'] = 'perm';
  $handler->display->display_options['cache']['type'] = 'none';
  $handler->display->display_options['query']['type'] = 'views_query';
  $handler->display->display_options['exposed_form']['type'] = 'basic';
  $handler->display->display_options['pager']['type'] = 'full';
  $handler->display->display_options['style_plugin'] = 'default';
  $handler->display->display_options['row_plugin'] = 'fields';
  /* Empty text: Global: Text area */
  $handler->display->display_options['empty']['text']['id'] = 'area';
  $handler->display->display_options['empty']['text']['table'] = 'views';
  $handler->display->display_options['empty']['text']['field'] = 'area';
  $handler->display->display_options['empty']['text']['empty'] = FALSE;
  $handler->display->display_options['empty']['text']['content'] = 'There are no items for this feed at the moment.';
  $handler->display->display_options['empty']['text']['format'] = '1';
  /* Relationship: Feeds Item: Owner feed */
  $handler->display->display_options['relationships']['feed_nid']['id'] = 'feed_nid';
  $handler->display->display_options['relationships']['feed_nid']['table'] = 'feeds_node_item';
  $handler->display->display_options['relationships']['feed_nid']['field'] = 'feed_nid';
  $handler->display->display_options['relationships']['feed_nid']['required'] = 1;
  /* Field: Feeds Item: Item URL */
  $handler->display->display_options['fields']['url']['id'] = 'url';
  $handler->display->display_options['fields']['url']['table'] = 'feeds_node_item';
  $handler->display->display_options['fields']['url']['field'] = 'url';
  $handler->display->display_options['fields']['url']['label'] = '';
  $handler->display->display_options['fields']['url']['exclude'] = TRUE;
  $handler->display->display_options['fields']['url']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['url']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['url']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['url']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['url']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['url']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['url']['alter']['html'] = 0;
  $handler->display->display_options['fields']['url']['hide_empty'] = 0;
  $handler->display->display_options['fields']['url']['empty_zero'] = 0;
  $handler->display->display_options['fields']['url']['display_as_link'] = 1;
  /* Field: Node: Title */
  $handler->display->display_options['fields']['title']['id'] = 'title';
  $handler->display->display_options['fields']['title']['table'] = 'node';
  $handler->display->display_options['fields']['title']['field'] = 'title';
  $handler->display->display_options['fields']['title']['label'] = '';
  $handler->display->display_options['fields']['title']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['title']['alter']['make_link'] = 1;
  $handler->display->display_options['fields']['title']['alter']['path'] = '[url]';
  $handler->display->display_options['fields']['title']['alter']['prefix'] = '<h2>';
  $handler->display->display_options['fields']['title']['alter']['suffix'] = '</h2>';
  $handler->display->display_options['fields']['title']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['title']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['title']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['title']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['title']['alter']['html'] = 0;
  $handler->display->display_options['fields']['title']['hide_empty'] = 0;
  $handler->display->display_options['fields']['title']['empty_zero'] = 0;
  $handler->display->display_options['fields']['title']['link_to_node'] = 0;
  /* Field: Node: Title */
  $handler->display->display_options['fields']['title_1']['id'] = 'title_1';
  $handler->display->display_options['fields']['title_1']['table'] = 'node';
  $handler->display->display_options['fields']['title_1']['field'] = 'title';
  $handler->display->display_options['fields']['title_1']['relationship'] = 'feed_nid';
  $handler->display->display_options['fields']['title_1']['label'] = '';
  $handler->display->display_options['fields']['title_1']['exclude'] = TRUE;
  $handler->display->display_options['fields']['title_1']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['title_1']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['title_1']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['title_1']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['title_1']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['title_1']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['title_1']['alter']['html'] = 0;
  $handler->display->display_options['fields']['title_1']['hide_empty'] = 0;
  $handler->display->display_options['fields']['title_1']['empty_zero'] = 0;
  $handler->display->display_options['fields']['title_1']['link_to_node'] = 1;
  /* Field: Global: Custom text */
  $handler->display->display_options['fields']['nothing']['id'] = 'nothing';
  $handler->display->display_options['fields']['nothing']['table'] = 'views';
  $handler->display->display_options['fields']['nothing']['field'] = 'nothing';
  $handler->display->display_options['fields']['nothing']['label'] = '';
  $handler->display->display_options['fields']['nothing']['alter']['text'] = 'From [title_1]';
  $handler->display->display_options['fields']['nothing']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['nothing']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['nothing']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['nothing']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['nothing']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['nothing']['alter']['html'] = 0;
  $handler->display->display_options['fields']['nothing']['hide_empty'] = 0;
  $handler->display->display_options['fields']['nothing']['empty_zero'] = 0;
  /* Field: Fields: field_body */
  $handler->display->display_options['fields']['entity_id']['id'] = 'entity_id';
  $handler->display->display_options['fields']['entity_id']['table'] = 'field_data_field_body';
  $handler->display->display_options['fields']['entity_id']['field'] = 'entity_id';
  $handler->display->display_options['fields']['entity_id']['label'] = '';
  $handler->display->display_options['fields']['entity_id']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['entity_id']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['entity_id']['alter']['absolute'] = 0;
  $handler->display->display_options['fields']['entity_id']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['entity_id']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['entity_id']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['entity_id']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['entity_id']['alter']['html'] = 0;
  $handler->display->display_options['fields']['entity_id']['hide_empty'] = 1;
  $handler->display->display_options['fields']['entity_id']['empty_zero'] = 0;
  /* Field: Node: Edit link */
  $handler->display->display_options['fields']['edit_node']['id'] = 'edit_node';
  $handler->display->display_options['fields']['edit_node']['table'] = 'node';
  $handler->display->display_options['fields']['edit_node']['field'] = 'edit_node';
  $handler->display->display_options['fields']['edit_node']['label'] = '';
  $handler->display->display_options['fields']['edit_node']['alter']['alter_text'] = 0;
  $handler->display->display_options['fields']['edit_node']['alter']['make_link'] = 0;
  $handler->display->display_options['fields']['edit_node']['alter']['trim'] = 0;
  $handler->display->display_options['fields']['edit_node']['alter']['word_boundary'] = 1;
  $handler->display->display_options['fields']['edit_node']['alter']['ellipsis'] = 1;
  $handler->display->display_options['fields']['edit_node']['alter']['strip_tags'] = 0;
  $handler->display->display_options['fields']['edit_node']['alter']['html'] = 0;
  $handler->display->display_options['fields']['edit_node']['hide_empty'] = 0;
  $handler->display->display_options['fields']['edit_node']['empty_zero'] = 0;
  $handler->display->display_options['fields']['edit_node']['text'] = 'Edit';
  /* Argument: Node: Nid */
  $handler->display->display_options['arguments']['nid']['id'] = 'nid';
  $handler->display->display_options['arguments']['nid']['table'] = 'node';
  $handler->display->display_options['arguments']['nid']['field'] = 'nid';
  $handler->display->display_options['arguments']['nid']['relationship'] = 'feed_nid';
  $handler->display->display_options['arguments']['nid']['default_action'] = 'empty';
  $handler->display->display_options['arguments']['nid']['style_plugin'] = 'default_summary';
  $handler->display->display_options['arguments']['nid']['title'] = 'Articles from %1';
  $handler->display->display_options['arguments']['nid']['default_argument_type'] = 'fixed';
  $handler->display->display_options['arguments']['nid']['break_phrase'] = 0;
  $handler->display->display_options['arguments']['nid']['not'] = 0;
  /* Filter: Node: Type */
  $handler->display->display_options['filters']['type']['id'] = 'type';
  $handler->display->display_options['filters']['type']['table'] = 'node';
  $handler->display->display_options['filters']['type']['field'] = 'type';
  $handler->display->display_options['filters']['type']['value'] = array(
    'feed_item' => 'feed_item',
  );
  $handler->display->display_options['filters']['type']['expose']['operator'] = FALSE;
  
/* Display: Page */
  $handler = $view->new_display('page', 'Page', 'page_1');
  $handler->display->display_options['defaults']['arguments'] = FALSE;
  /* Argument: Node: Nid */
  $handler->display->display_options['arguments']['nid']['id'] = 'nid';
  $handler->display->display_options['arguments']['nid']['table'] = 'node';
  $handler->display->display_options['arguments']['nid']['field'] = 'nid';
  $handler->display->display_options['arguments']['nid']['relationship'] = 'feed_nid';
  $handler->display->display_options['arguments']['nid']['default_action'] = 'empty';
  $handler->display->display_options['arguments']['nid']['style_plugin'] = 'default_summary';
  $handler->display->display_options['arguments']['nid']['title'] = 'All items from %1';
  $handler->display->display_options['arguments']['nid']['default_argument_type'] = 'fixed';
  $handler->display->display_options['arguments']['nid']['validate_type'] = 'node';
  $handler->display->display_options['arguments']['nid']['validate_options']['types'] = array(
    'feed' => 'feed',
    'feed_item' => 0,
    'article' => 0,
  );
  $handler->display->display_options['arguments']['nid']['break_phrase'] = 0;
  $handler->display->display_options['arguments']['nid']['not'] = 0;
  $handler->display->display_options['path'] = 'node/%/feed-items';
  $handler->display->display_options['menu']['type'] = 'tab';
  $handler->display->display_options['menu']['title'] = 'View items';
  $handler->display->display_options['menu']['weight'] = '0';

  $views[$view->name] = $view;

  return $views;
}
