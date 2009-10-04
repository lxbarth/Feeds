<?php
// $Id: feeds_ui.admin.inc,v 1.14 2009/09/21 14:55:47 alexb Exp $
/**
 * @file
 */

/**
 * Build overview of available configurations.
 */
function feeds_ui_overview_page() {

  $rows = array();
  if ($feeds = feeds_load_all()) {
    foreach ($feeds as $feed) {
      if (empty($feed->config['content_type'])) {
        $type = '[none]';
      }
      else {
        $type = l(node_get_types('name', $feed->config['content_type']), 'node/add/'. $feed->config['content_type']);
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
function feeds_ui_create_form(&$form_state) {
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
function feeds_ui_create_form_validate($form, &$form_state) {
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
function feeds_ui_create_form_submit($form, &$form_state) {
  // Create feed.
  $feed = feeds_instance('Feed', $form_state['values']['id']);
  $feed->addConfig($form_state['values']);
  $feed->save();

  // Set a message and redirect to settings form.
  drupal_set_message(t('Your feed has been created with default settings. If they do not fit your use case you can change them here.'));
  $form_state['redirect'] = 'admin/build/feeds/edit/'. $feed->id;
  menu_rebuild();
}

/**
 * Edit feed configuration.
 */
function feeds_ui_edit_page($feed, $active = 'help', $plugin_key = '') {

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
      $active_container['body'] = drupal_get_form('feeds_ui_mapping_form', $feed, $plugin_key);
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

  return theme('feeds_ui_edit_page', $config_info, $active_container);
}

/**
 * Helper for building a list of plugins of a specific type.
 * 
 * @todo: this code may be able to simplify by creating a better API on Feed class for
 * manipulating fetcher/parser/processor plugins. Right now we always need to know which
 * type of plugin we are manipulating which creates a lot of awkward code.
 * 
 * @param $feed
 *   Feed object.
 * @param $type
 *   Plugin type. One of 'fetcher', 'parser', 'processor'.
 * @param $selected
 *   The plugin(s) that are currently selected.
 * @param $multiple
 *   If TRUE, allow multiple choice.
 * 
 * @return 
 *   An array themeable by theme_feeds_ui_container().
 */
function _feeds_ui_list_plugins_by_type($feed, $type, $selected, $multiple = FALSE) {
  $plugins = feeds_get_plugins_by_type();
  $plugins = $plugins[$type];

  $containers = array();
  foreach ($plugins as $key => $plugin) {

    $actions = array();
    if (in_array($key, $selected)) {
      if ($multiple) {
        $param = array(
          'feed_id' => $feed->id, 
          'plugin_key' => $key,
        );
        $actions = array(feeds_ui_get_form('feeds_ui_button_form', t('Remove'), 'feeds_ui_remove_plugin', $param));
      }
    }
    else {
      $param = array(
        'feed_id' => $feed->id, 
        'plugin_key' => $key,
      );
      $actions = array(feeds_ui_get_form('feeds_ui_button_form', t('Select'), 'feeds_ui_select_plugin', $param));
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
 * Select callback, helper for _feeds_ui_list_plugins_by_type().
 */
function feeds_ui_select_plugin($args) {
  $plugin_key = $args['plugin_key'];
  $feed = feeds_instance('Feed', $args['feed_id']);

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

/**
 * Remove callback, helper for _feeds_ui_list_plugins_by_type().
 */
function feeds_ui_remove_plugin($args) {
  $plugin_key = $args['plugin_key'];
  $feed = feeds_instance('Feed', $args['feed_id']);

  if (feeds_plugin_child($plugin_key, 'FeedsProcessor')) {
    $feed->removeProcessor($plugin_key);
    $feed->save();
  }
}

/**
 * Wrapper or drupal_get_form().
 * 
 * Generate a form with a unique, anonymous form_id. Use like drupal_get_form()
 * but build as many same forms on one page as you'd like AND submit them 
 * independently. Make sure that forms that are being built with this wrapper
 * declare their validation and submit handlers explicitely ($form['#validate']
 * and $form['#submit']).
 * 
 * @see drupal_get_form()
 * @see feeds_ui_register_form()
 * 
 * @param $form_callback
 *   Form callback for this form.
 * @param ... Parameters to pass into form
 */
function feeds_ui_get_form($form_callback) {
  $args = func_get_args();
  array_shift($args);

  // Register the form callback generates a random id and exposes the form id to FormAPI.
  $form_id = feeds_ui_register_form($form_callback);
  array_unshift($args, $form_id);

  // Build form with drupal_get_form().
  return call_user_func_array('drupal_get_form', $args);
}

/**
 * Form that has one button that fires an action. 
 * 
 * Use together with feeds_ui_get_form for using multiple times on one page.
 * 
 * $output = feeds_ui_get_form('feeds_ui_button_form', t('Select'), 'call_back_func', $param);
 * 
 * @param $form_state
 *   Form API form state. 
 * @param $label
 *   Label on the button.
 * @param $callback 
 *   Callback to invoke when button is pushed.
 * @param $param
 *   Parameter to pass to callback.
 */
function feeds_ui_button_form(&$form_state, $label, $callback, $param) {
  $form = array();
  $form['#button_callback'] = $callback;
  $form['#callback_param'] = $param;
  $form['#submit'] = array('feeds_ui_button_form_submit');
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => $label,
  );
  return $form;
}

/**
 * Submit handler for feeds_ui_button_form().
 */
function feeds_ui_button_form_submit($form, &$form_state) {
  if (isset($form['#button_callback'])) {
    $callback = $form['#button_callback'];
    $param = $form['#callback_param'];
    $callback($param);
  }
}

/**
 * Edit mapping.
 */
function feeds_ui_mapping_form(&$form_state, $feed, $processor_key) {
  $processor = $feed->processors[$processor_key];

  $form = array();
  $form['#feed'] = $feed;
  $form['#processor'] = $processor;

  // Get mapping sources from parsers and targets from processor.
  $targets = $processor->getMappingTargets();
  $sources = $feed->parser->getMappingSources();

  // Add unique and remove forms to mappings.
  $mappings = $processor->getMappings();
  foreach ($mappings as $k => $mapping) {
    $param = array(
      'processor' => $processor, 
      'mapping' => $mapping,
    );
    $mappings[$k]['unique'] = feeds_ui_get_form('feeds_ui_button_form', $mapping['unique'] ? t('Yes') : t('No'), 'feeds_ui_toggle_unique', $param);
    $mappings[$k]['remove'] = feeds_ui_get_form('feeds_ui_button_form', t('Remove'), 'feeds_ui_remove_mapping', $param);
  }

  $form['#mappings'] = $mappings;
  $form['#targets'] = $targets;
  if ($sources) {
    $form['source'] = array(
      '#type' => 'select',
      '#options' => array('' => t('Select a source')) + drupal_map_assoc($sources),
    );
  }
  else {
    drupal_add_js(drupal_get_path('module', 'feeds_ui') .'/feeds_ui.js');
    $form['source'] = array(
      '#type' => 'textfield',
      '#size' => 20,
      '#default_value' => t('Name of source field'),
      '#attributes' => array('class' => 'hide-text-on-focus'),
    );
  }
  $form['target'] = array(
    '#type' => 'select',
    '#options' => array('' => t('Select a target')) + drupal_map_assoc(array_keys($targets)),
  );
  $form['add'] = array(
    '#type' => 'submit',
    '#value' => t('Add'),
  );
  return $form;
}

/**
 * Submit handler for feeds_ui_mapping_form().
 */
function feeds_ui_mapping_form_submit($form, &$form_state) {
  $processor = $form['#processor'];
  $processor->addMapping($form_state['values']['source'], $form_state['values']['target']);
  $processor->save();
}

/**
 * Toggle unique callback for mapping form.
 */
function feeds_ui_toggle_unique($args) {
  $processor = $args['processor'];
  $processor->setUnique($args['mapping']['source'], $args['mapping']['target'], !$args['mapping']['unique']);
  $processor->save();
}

/**
 * Remove callback for mapping form.
 */
function feeds_ui_remove_mapping($args) {
  $processor = $args['processor'];
  $processor->removeMapping($args['mapping']['source'], $args['mapping']['target']);
  $processor->save();
}

/**
 * Delete configuration form.
 * 
 * @todo: move up under create form.
 */
function feeds_ui_delete_form(&$form_state, $feed) {
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
 * Submit handler for feeds_ui_delete_form().
 */
function feeds_ui_delete_form_submit($form, &$form_state) {
  feeds_delete($form['#feed']->id);
}

/**
 * Contextual help for build/edit page.
 * 
 * @todo: Write.
 * @todo: Move up closer to edit page callback.
 */
function feeds_ui_edit_help() {
  return 'Every feed must have at least one fetcher, parser and processor.';
}

/**
 * Theme feeds_ui_edit_page().
 */
function theme_feeds_ui_edit_page($config_info, $active_container) {
  drupal_add_css(drupal_get_path('module', 'feeds_ui') .'/feeds_ui.css');

  // Outer wrapper.
  $output = '<div class="feeds-settings">';

  // Build left bar.
  $output .= '<div class="left-bar">';
  foreach ($config_info as $info) {
    $output .= theme('feeds_ui_container', $info);
  }
  $output .= '</div>';

  // Build configuration space.
  $output .= '<div class="configuration">';
  $output .= '<div class="configuration-squeeze">';
  $output .= theme('feeds_ui_container', $active_container);
  $output .= '</div>';
  $output .= '</div>';

  $output .= '</div>'; // ''<div class="feeds-settings">';

  return $output;
}

/**
 * Render a simple container. A container can have a title, a description and 
 * one or more actions. Recursive.
 * 
 * @todo: Replace with theme_fieldset or a wrapper to theme_fieldset?
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
 * Theme function for feeds_ui_mapping_form().
 */
function theme_feeds_ui_mapping_form($form) {

  $header = array(
    t('Source'),
    t('Target'),
    t('Unique'),
    '&nbsp;',
  );

  $rows = array();
  if (is_array($form['#mappings'])) {
    foreach ($form['#mappings'] as $mapping) {
      $rows[] = array(
        $mapping['source'],
        $mapping['target'],
        // @todo: only second form renders correctly. FIX.
        '<div style="display:none">'. $mapping['unique'] .'</div>'. $mapping['unique'],
        $mapping['remove'],
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
    drupal_render($form['source']),
    drupal_render($form['target']),
    '',
    drupal_render($form['add']),
  );
  $output = theme('table', $header, $rows);
  $output .= drupal_render($form);
  return $output;
}