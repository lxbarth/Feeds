<?php
// $Id: feeds_ui.admin.inc,v 1.14 2009/09/21 14:55:47 alexb Exp $
/**
 * @file
 */

/**
 * List available feed configurations.
 */
function feeds_ui_content_overview() {
  $rows = array();
  if ($feeds = feeds_load_all()) {
    foreach ($feeds as $feed) {
      $rows[] = array(
        $feed->id,
      );
    }
  }
  $header = array(
    t('Configurations'),
  );
  return theme('table', $header, $rows);
}

/**
 * Build overview of available configurations.
 */
function feeds_ui_build_overview() {
  $rows = array();
  if ($feeds = feeds_load_all()) {
    foreach ($feeds as $feed) {
      $config = $feed->config;
      if (empty($config['content_type'])) {
        $type = '[none]';
      }
      else {
        $type = l(node_get_types('name', $config['content_type']), 'node/add/'. $config['content_type']);
      }
      $rows[] = array(
        $feed->config['name'],
        $type,
        l(t('Edit'), 'admin/build/feeds/edit/'. $feed->id) .' | '. 
        l(t('Delete'), 'admin/build/feeds/delete/'. $feed->id),
      );
    }
  }
  $rows[] = array(
    l(t('New configuration'), 'admin/build/feeds/create'),
    '&nbsp;',
    '&nbsp;',
  );
  $header = array(
    t('Configuration'),
    t('Attached to'),
    t('Operations'),
  );
  return theme('table', $header, $rows);
}

/**
 * Create a new configuration.
 * 
 * @todo: make this a multi-step once all forms are stabilized.
 */
function feeds_ui_build_create_form(&$form_state) {
  drupal_add_js(drupal_get_path('module', 'feeds_ui') .'/feeds_ui.js');
  $form = array();
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('A natural name for this configuration. Example: RSS Feed. You can always change this name later.'),
    '#required' => TRUE,
    '#attributes' => array('class' => 'feed-name'),
  );
  $form['id'] = array(
    '#type' => 'textfield',
    '#title' => t('Machine name'),
    '#description' => t('A unique identifier for this configuration. Example: rss_feed. Must only contain lower case characters, numbers and underscores.'),
    '#required' => TRUE,
    '#attributes' => array('class' => 'feed-id'),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Create'),
  );
  return $form;
}

/**
 * Validation handler for feeds_build_create_form().
 */
function feeds_ui_build_create_form_validate($form, &$form_state) {
  ctools_include('export');
  $feed = feeds_instance('Feed', $form_state['values']['id']);
  if (ctools_export_load_object('feeds_config', 'conditions', array('id' => $form_state['values']['id'], 'class' => 'Feed'))) {
    form_set_error('id', t('Id is taken.'));
  }
  $feed->configFormValidate($form, &$form_state);
}

/**
 * Submit handler for feeds_build_create_form().
 */
function feeds_ui_build_create_form_submit($form, &$form_state) {
  // Create feed.
  $feed = feeds_instance('Feed', $form_state['values']['id']);
  $feed->addConfig($form_state['values']);
  $feed->save();

  // Set a message and redirect to settings form.
  drupal_set_message(t('Your feed has been created with default settings. If they do not fit your use case you can change them here.'));
  $form_state['redirect'] = 'admin/build/feeds/edit/'. $feed->id;
}

/**
 * Edit feed configuration.
 */
function feeds_ui_build_edit_page($feed, $active = 'help', $plugin_key = '') {

  // Get plugins and configuration.
  $plugins = feeds_get_plugins();
  $config = $feed->config;  
  // Base path for changing the active container.
  $path = 'admin/build/feeds/edit/'. $feed->id .'/';

  $active_container = array(
    'class' => 'active-container',
  );
  switch ($active) {
    case 'help':
      $active_container['title'] = t('Help');
      $active_container['body'] = feeds_ui_edit_help();
      break;
    case 'fetcher':
      $active_container['title'] = t('Select a fetcher');
      $active_container['body'] = _feeds_ui_list_plugins_by_type($feed, 'fetcher', array($config['fetcher']));
      break;
    case 'parser':
      $active_container['title'] = t('Select a parser');
      $active_container['body'] = _feeds_ui_list_plugins_by_type($feed, 'parser', array($config['parser']));
      break;
    case 'processor':
      $active_container['title'] = t('Add a processor');
      $active_container['body'] = _feeds_ui_list_plugins_by_type($feed, 'processor', $config['processors'], TRUE);
      break;
    case 'settings':
      // @todo: Use central factory and simplify this.
      if ($plugin_key == 'Feed') {
        $active_container['title'] = t('Basic settings');
        $active_container['body'] = feeds_get_config_form($feed);
      }
      elseif ($class = feeds_load_plugin_class($plugin_key)) {
        $plugin = feeds_instance($class, $feed->id);
        $active_container['title'] = t('Settings for !plugin', array('!plugin' => $plugins[$plugin_key]['name']));
        $active_container['body'] = feeds_get_config_form($plugin);
      }
      break;
    case 'mapping':
      $active_container['title'] = t('Mapping for !processor', array('!processor' => $plugins[$plugin_key]['name']));
      $active_container['body'] = drupal_get_form('feeds_ui_build_mapping_form', $feed, $plugin_key);
      break;
  }

  // Build config info.
  $config_info = $info = array();
  $info['class'] = 'config-set';

  // Basic information.
  $info['title'] = t('Basic settings');
  $info['body'] = array(
    array(
      'body' => '@todo: summary',
      'actions' => array(l(t('Settings'), $path .'settings/Feed')),
    ),
  );
  $config_info[] = $info;

  // Fetcher.
  $fetcher = $plugins[$config['fetcher']];
  $actions = array();
  if (feeds_get_config_form($feed->fetcher)) {
    $actions = array(l(t('Settings'), $path .'settings/'. $config['fetcher']));
  }
  $info['title'] = t('Fetcher');
  $info['body'] = array(
    array(
      'title' => $fetcher['name'],
      'body' => $fetcher['description'],
      'actions' => $actions,
    ),
  );
  $info['actions'] = array(l(t('Change'), $path .'fetcher'));
  $config_info[] = $info;

  // Parser.
  $parser = $plugins[$config['parser']];
  $actions = array();
  if (feeds_get_config_form($feed->parser)) {
    $actions = array(l(t('Settings'), $path .'settings/'. $config['parser']));
  }
  $info['title'] = t('Parser');
  $info['body'] = array(
    array(
      'title' => $parser['name'],
      'body' => $parser['description'],
      'actions' => $actions,
    )
  );
  $info['actions'] = array(l(t('Change'), $path .'parser'));
  $config_info[] = $info;

  // Processor.
  $info['title'] = t('Processor');
  $info['body'] = array();
  $info['actions'] = array(l(t('Change'), $path .'processor'));
  foreach ($config['processors'] as $processor) {
    $actions = array();
    if (feeds_get_config_form($feed->processors[$processor])) {
      $actions[] = l(t('Settings'), $path .'settings/'. $processor);
    }
    $actions[] = l(t('Mapping'), $path .'mapping/'. $processor);

    $info['body'][] = array(
      'title' => $plugins[$processor]['name'],
      'body' => $plugins[$processor]['description'],
      'actions' => $actions,
    );
  }
  $config_info[] = $info;

  return theme('feeds_ui_build_edit_page', $config_info, $active_container);
}

/**
 * Helper for building a list of plugins of a specific type.
 * 
 * @param $feed
 *   Feed object.
 * @param $type
 *   Plugin type. One of 'fetcher', 'parser', 'processor'.
 * @param $selected
 *   The plugin(s) that are currently selected.
 * @param $multiple
 *   If TRUE, allow multiple choice.
 */
function _feeds_ui_list_plugins_by_type($feed, $type, $selected, $multiple = FALSE) {
  $plugins = feeds_get_plugins_by_type();
  $plugins = $plugins[$type];

  if ($multiple) {
    
  }
  
  $containers = array();
  foreach ($plugins as $key => $plugin) {

    $actions = array();
    if (in_array($key, $selected)) {
      if ($multiple) {
        $actions = array(_feeds_ui_get_select_form($feed, $plugin['handler']['class'], 'remove'));
      }
    }
    else {
      $actions = array(_feeds_ui_get_select_form($feed, $plugin['handler']['class'], 'select'));
    }

    $containers[] = array(
      'title' => $plugin['name'],
      'body' => $plugin['description'],
      'actions' => $actions,
    );
  }
  return $containers;
}

/**
 * Helper for getting a feeds_ui_select_form().
 * 
 * @param $feed
 *   A Feed object.
 * @param $plugin_key
 *   A string that identifies a Feeds plugin.
 * @param $action
 *   Either 'select' or 'remove'. 
 * 
 * @return 
 *   A rendered form.
 */
function _feeds_ui_get_select_form($feed, $plugin_key, $action) {
  return drupal_get_form($plugin_key .'_feeds_ui_select_form', $feed, $plugin_key, $action);
}

/**
 * Form callback for selecting a plugin on a feed.
 */
function feeds_ui_select_form(&$form_state, $feed, $plugin_key, $action = 'select') {
  $form = array();
  $form['#feed'] = $feed;
  $form['#plugin_key'] = $plugin_key;
  $form['#submit'] = array('feeds_ui_select_form_submit');
  if ($action == 'remove') {
    $form['remove'] = array(
      '#type' => 'submit',
      '#value' => t('Remove'),
    );
  }
  else {
    $form['select'] = array(
      '#type' => 'submit',
      '#value' => t('Select'),
    );
  }
  return $form;
}

/**
 * Submit handler for feeds_ui_select_form().
 */
function feeds_ui_select_form_submit($form, &$form_state) {
  $plugin_key = $form['#plugin_key'];
  $feed = $form['#feed'];
  if (isset($form['select'])) {
    if (feeds_plugin_child($plugin_key, 'FeedsFetcher')) {
      $feed->setFetcher($plugin_key);
      $feed->save();
    }
    elseif (feeds_plugin_child($plugin_key, 'FeedsParser')) {
      $feed->setParser($plugin_key);
      $feed->save();
    }
    elseif (feeds_plugin_child($plugin_key, 'FeedsProcessor')) {
      $feed->addProcessor($plugin_key);
      $feed->save();
    }    
  }
  elseif (isset($form['remove'])) {
    if (feeds_plugin_child($plugin_key, 'FeedsProcessor')) {
      $feed->removeProcessor($plugin_key);
      $feed->save();
    }
  }
}

/**
 * Edit mapping.
 */
function feeds_ui_build_mapping_form(&$form_state, $feed, $processor) {
  $form = array();
  $form['#feed'] = $feed;

  // Build a mapping form for each processor configured.
  $sources = $feed->parser->getMappingSources();
  $processor = $feed->processors[$processor];

  // @todo: move actual form building into processors?
  $mappings = $processor->getMappings();
  $targets = $processor->getMappingTargets();
  $form['processors'][$class] = array(
    '#type' => 'fieldset',
    '#title' => $class, // @todo: human readable title.
    '#tree' => TRUE,
  );
  $form['processors'][$class]['#mappings'] = $mappings;
  $form['processors'][$class]['#targets'] = $targets;
  $form['processors'][$class]['source'] = array(
    '#type' => 'select',
    '#options' => array('' => t('Select a source')) + drupal_map_assoc($sources),
  );
  $form['processors'][$class]['target'] = array(
    '#type' => 'select',
    '#options' => array('' => t('Select a target')) + drupal_map_assoc(array_keys($targets)),
  );
  $form['processors'][$class]['add'] = array(
    '#type' => 'submit',
    '#value' => t('Add'),
  );
  return $form;
}

/**
 * Submit handler for feeds_ui_build_mapping_form().
 */
function feeds_ui_build_mapping_form_submit($form, &$form_state) {
  foreach ($form['#feed']->processors as $class => $processor) {
    $processor->addMapping($form_state['values'][$class]['source'], $form_state['values'][$class]['target']);
  }
}

/**
 * Remove a mapping.
 */
function feeds_ui_build_mapping_remove($feed, $processor, $target, $token) {
  if (drupal_valid_token($token, $target)) {
    $mapping = $feed->processors[$processor]->removeMapping($target);
  }
  else {
    drupal_set_message(t('Invalid token.'), 'error');
  }
  drupal_goto('admin/build/feeds/edit/'. $feed->id .'/map');
}

/**
 * Delete form.
 */
function feeds_ui_build_delete_form(&$form_state, $feed) {
  $form['#redirect'] = 'admin/build/feeds';
  $form['#feed'] = $feed;
  return confirm_form($form,
    t('Would you really like to delete the configuration !id?', array('!id' => $feed->id)),
    $form['#redirect'],
    t('This action cannot be undone.'),
    t('Delete')
  );
}

/**
 * Submit handler for feeds_ui_build_delete_form().
 */
function feeds_ui_build_delete_form_submit($form, &$form_state) {
  feeds_delete($form['#feed']->id);
}

/**
 * Contextual help for build/edit page.
 */
function feeds_ui_edit_help() {
  return 'Every feed must have at least one fetcher, parser and processor.';
}

/**
 * Theme feeds_ui_build_edit_page().
 */
function theme_feeds_ui_build_edit_page($config_info, $active_container) {
  drupal_add_css(drupal_get_path('module', 'feeds_ui') .'/feeds_ui.css');
  drupal_add_js(drupal_get_path('module', 'feeds_ui') .'/feeds_ui.js');

  // Outer wrapper.
  $output = '<div class="feeds-settings">';

  // Build left bar.
  $output .= '<div class="left-bar">';
  foreach ($config_info as $info) {
    $output .= theme('feeds_ui_container', $info);
  }
  $output .= '</div>'; // '<div class="left-bar">'

  // Build configuration space.
  $output .= '<div class="configuration">';
  $output .= '<div class="configuration-squeeze">';
  $output .= theme('feeds_ui_container', $active_container);
  $output .= '</div>'; // '<div class="configuration-squeeze">'
  $output .= '</div>'; // '<div class="configuration">'

  $output .= '</div>'; // ''<div class="feeds-settings">';

  return $output;
}

/**
 * Render a simple container. A container can have a title, a description and 
 * one or more actions.
 * 
 * @param $container 
 *   An array that describes the container. All elements are optional:
 *   array(
 *     'title' => 'the title',
 *     'body' => 'the body of the container, may also be an ARRAY of more containers.',
 *     'class' => 'the class of the container.',
 *     'id' => 'the id of the container',
 *   );
 */
function theme_feeds_ui_container($container) {

  $class = empty($container['class']) ? ' plain': " {$container['class']}";
  $id = empty($container['id']) ? '': ' id="'. $container['id'] .'"';
  $output = '<div class="feeds-container'. $class .'"'. $id .'>';

  if (count($container['actions'])) {
    $output .= theme('item_list', $container['actions'], NULL, 'ul', array('class' => 'container-actions'));
  }

  if (!empty($container['title'])) {
    $output .= '<h4 class="feeds-container-title">';
    $output .= $container['title'];
    $output .= '</h4>';
  }

  if (!empty($container['body'])) {
    $output .= '<div class="feeds-container-body">';
    if (is_array($container['body'])) {
      foreach ($container['body'] as $c) {
        $output .= theme('feeds_ui_container', $c);
      }
    }
    else {
      $output .= $container['body'];
    }
    $output .= '</div>';
  }

  $output .= '</div>';
  return $output;
}

/**
 * Theme function for feeds_ui_build_mapping_form().
 */
function theme_feeds_ui_build_mapping_form($form) {
  $output = '';
  $header = array(
    t('Source'),
    t('Target'),
    t('Unique'),
    '&nbsp;',
  );

  foreach (element_children($form['processors']) as $processor) {
    $rows = array();
    if (is_array($form['processors'][$processor]['#mappings'])) {
      foreach ($form['processors'][$processor]['#mappings'] as $target => $mapping) {
        $rows[] = array(
          $mapping['source'],
          $target,
          $mapping['unique'] ? t('Yes') : t('No'),
          l(t('Remove'), 'admin/build/feeds/map-remove/'. $form['#feed']->id .'/'. $processor .'/'. $target .'/'. drupal_get_token($target)),
        ); 
      }
    }
    if (!count($rows)) {
      $rows[] = array(
        array(
          'colspan' => 4,
          'data' => t('No mappings defined.'),
        ),
      );
    }
    $rows[] = array(
      drupal_render($form['processors'][$processor]['source']),
      drupal_render($form['processors'][$processor]['target']),
      '',
      drupal_render($form['processors'][$processor]['add']),
    );
    $form['processors'][$processor]['#value'] = theme('table', $header, $rows);
  }
  $output .= drupal_render($form);
  return $output;
}