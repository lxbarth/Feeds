<?php
// $Id: feeds_ui.admin.inc,v 1.14 2009/09/21 14:55:47 alexb Exp $
/**
 * @file
 * Contains all page callbacks, forms and theming functions for Feeds
 * administrative pages.
 */

/**
 * Introductory help for admin/build/feeds/edit/config page
 */
function feeds_ui_edit_help() {
  return t('
    <p>
    You can create as many feed configurations as you would like to. Every one of them can have a distinct purpose like letting your users aggregate RSS feeds or importing a CSV file for content migration. Here are a couple of things that are important to understand in order to get started with Feeds:
    </p>
    <ul>
    <li>
    Every feed configuration consists of basic settings, a fetcher, a parser and a processor and their settings.
    </li>
    <li>
    The <strong>basic settings</strong> define the general behavior of the feed. <strong>Fetchers</strong> are responsible for loading data, <strong>parsers</strong> for organizing it and <strong>processors</strong> for "doing stuff" with it, usually storing it.
    </li>
    <li>
    In Basic settings, you can <strong>attach a feed configuration to a content type</strong>. This is for example useful for enabling your users to create as many feeds of a configuration as they would like to. If you don\'t attach a configuration to a content type, you can use it on the !import page.
    </li>
    <li>
    Feeds can be <strong>refreshed periodically</strong> - see the minimum refresh period in the Basic settings.
    </li>
    <li>
    Processors can have <strong>mappings</strong> in addition to settings. Mappings allow you to define what elements of a feed should be mapped to what content fields on a granular level. For instance, you can specify that a feed\'s author should be mapped to a node\'s body.
    </li>
    </ul>
    ', array('!import' => l(t('Import'), 'import')));
}

/**
 * Help text for mapping.
 */
function feeds_ui_mapping_help() {
  return t('
  <p>
  Define which elements of a single item of a feed (= <em>Sources</em>) map to which content pieces in Drupal (= <em>Targets</em>). Make sure that at least one definition has a <em>Unique target</em>. A unique target means that a value for a target can only occur once. E. g. only one item with the URL <em>http://example.com/story/1</em> can exist.
  </p>
  ');
}

/**
 * Build overview of available configurations.
 */
function feeds_ui_overview_page() {

  $rows = array();
  if ($feeds = feeds_importer_load_all()) {
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
        l(t('Export'), 'admin/build/feeds/export/'. $feed->id) .' | '.
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
  $form['description'] = array(
    '#type' => 'textfield',
    '#title' => t('Description'),
    '#description' => t('A description of this configuration.'),
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
  $feed = feeds_importer($form_state['values']['id']);
  if (ctools_export_load_object('feeds_config', 'conditions', array('id' => $form_state['values']['id']))) {
    form_set_error('id', t('Id is taken.'));
  }
  $feed->configFormValidate($form_state['values']);
}

/**
 * Submit handler for feeds_build_create_form().
 */
function feeds_ui_create_form_submit($form, &$form_state) {
  // Create feed.
  $feed = feeds_importer($form_state['values']['id']);
  $feed->addConfig($form_state['values']);
  $feed->save();

  // Set a message and redirect to settings form.
  drupal_set_message(t('Your feed has been created with default settings. If they do not fit your use case you can change them here.'));
  $form_state['redirect'] = 'admin/build/feeds/edit/'. $feed->id;
}

/**
 * Delete configuration form.
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
  // Delete feed.
  $form['#feed']->delete();

  // Clear cache, deleting a configuration may have an affect on menu tree.
  menu_rebuild();
}

/**
 * Export a feed configuration.
 */
function feeds_ui_export_form(&$form_state, $feed) {
  $code = feeds_export($feed->id);

  $form['export'] = array(
    '#title' => t('Export feed configuration'),
    '#type' => 'textarea',
    '#value' => $code,
    '#rows' => substr_count($code, "\n"),
  );
  return $form;
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

  $configure = t('!configure - ', array('!configure' => l(t('Configure your feed'), $path)));
  $active_container = array(
    'class' => 'active-container',
  );
  switch ($active) {
    case 'help':
      $active_container['title'] = t('Configure your feed');
      $active_container['body'] = '<div class="help feeds-admin-ui">'. feeds_ui_edit_help() .'</div>';
      break;
    case 'fetcher':
    case 'parser':
    case 'processor':
      $active_container['title'] = $configure . t('select a !plugin_type', array('!plugin_type' => $active));
      $active_container['body'] = _feeds_ui_list_plugins_by_type($feed, $active, $config[$active]['plugin_key']);
      break;
    case 'settings':
      if ($plugin_key == 'Feed') {
        $active_container['title'] = $configure . t('basic settings');
        $active_container['body'] = feeds_get_config_form($feed);
      }
      // feeds_plugin_instance() returns a correct result because feed has been
      // instantiated previously.
      elseif (in_array($plugin_key, array_keys($plugins)) && $plugin = feeds_plugin_instance($plugin_key, $feed->id)) {
        $active_container['title'] = $configure . t('settings for !plugin', array('!plugin' => $plugins[$plugin_key]['name']));
        $active_container['body'] = feeds_get_config_form($plugin);
      }
      break;
    case 'mapping':
      $active_container['title'] = $configure . t('mapping for !processor', array('!processor' => $plugins[$config['processor']['plugin_key']]['name']));
      $active_container['body'] = drupal_get_form('feeds_ui_mapping_form', $feed);
      break;
  }

  // Build config info.
  $config_info = $info = array();
  $info['class'] = 'config-set';

  // Basic information.
  $items = array();
  $items[] = t('Attached to: !type', array('!type' => $feed->config['content_type'] ? node_get_types('name', $feed->config['content_type']) : t('[none]')));
  if ($feed->config['refresh_period'] == FEEDS_SCHEDULE_NEVER) {
    $refresh_period = t('never');
  }
  elseif ($feed->config['refresh_period'] == 0) {
    $refresh_period = t('as often as possible');
  }
  else {
    $refresh_period = t('every !interval', array('!interval' => format_interval($feed->config['refresh_period'])));
  }
  $items[] = t('Refresh: !refresh_period', array('!refresh_period' => $refresh_period));
  $items[] = $feed->config['import_on_create'] ? t('Import on create') : t('Do not import on create');

  $info['title'] = t('Basic settings');
  $info['body'] = array(
    array(
      'body' => theme('item_list', $items),
      'actions' => array(l(t('Settings'), $path .'settings/Feed')),
    ),
  );
  $config_info[] = $info;

  // Fetcher.
  $fetcher = $plugins[$config['fetcher']['plugin_key']];
  $actions = array();
  if (feeds_get_config_form($feed->fetcher)) {
    $actions = array(l(t('Settings'), $path .'settings/'. $config['fetcher']['plugin_key']));
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
  $parser = $plugins[$config['parser']['plugin_key']];
  $actions = array();
  if (feeds_get_config_form($feed->parser)) {
    $actions = array(l(t('Settings'), $path .'settings/'. $config['parser']['plugin_key']));
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
  $processor = $plugins[$config['processor']['plugin_key']];
  $actions = array();
  if (feeds_get_config_form($feed->processor)) {
    $actions[] = l(t('Settings'), $path .'settings/'. $config['processor']['plugin_key']);
  }
  // @todo: We assume that every processor supports mapping - that's not
  // necessarily the case.
  $actions[] = l(t('Mapping'), $path .'mapping');
  $info['title'] = t('Processor');
  $info['body'] = array(
    array(
      'title' => $processor['name'],
      'body' => $processor['description'],
      'actions' => $actions,
    )
  );
  $info['actions'] = array(l(t('Change'), $path .'processor'));
  $config_info[] = $info;

  return theme('feeds_ui_edit_page', $config_info, $active_container);
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
 *
 * @return
 *   An array themeable by theme_feeds_ui_container().
 *
 * @todo: make this a single form. Simpletest can't push buttons on forms
 *   with the same label on submit buttons.
 */
function _feeds_ui_list_plugins_by_type($feed, $type, $selected) {
  $plugins = feeds_get_plugins_by_type();
  $plugins = $plugins[$type];

  $containers = array();
  foreach ($plugins as $key => $plugin) {
    $actions = array();
    if ($key != $selected) {
      $param = array(
        'feed_id' => $feed->id,
        'plugin_key' => $key,
      );
      $actions = array(feeds_ui_get_form('feeds_ui_button_form', 'Select', 'feeds_ui_select_plugin', $param));
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
  $feed = feeds_importer($args['feed_id']);

  // Set the plugin and save feed.
  $feed->setPlugin($plugin_key);
  $feed->save();
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

  // Register the form callback generates a random id and exposes the form id to
  // FormAPI.
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
 * $output =
 *   feeds_ui_get_form('feeds_ui_button_form', 'Select', 'call_back_func', $param);
 *
 * @param $form_state
 *   Form API form state.
 * @param $label
 *   Label on the button. Don't wrap in t().
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
    '#value' => t($label),
    '#attributes' => array('class' => 'feeds-ui-button'),
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
 *
 * @todo: completely merge this into config form handling. This is just a
 *   shared form of configuration, most of the common functionality can live in
 *   FeedsProcessor, a flag can tell whether mapping is supported or not.
 */
function feeds_ui_mapping_form(&$form_state, $feed) {
  drupal_add_js(drupal_get_path('module', 'feeds_ui') .'/feeds_ui.js');

  $form = array();
  $form['#feed'] = $feed;
  $form['help']['#value'] = feeds_ui_mapping_help();

  // Get mapping sources from parsers and targets from processor, format them
  // for output.
  // Some parsers do not define mapping sources but let them define on the fly.
  if ($sources = $feed->parser->getMappingSources()) {
    $source_options = _feeds_ui_format_options($sources);
  }
  $targets = $feed->processor->getMappingTargets();
  $target_options = _feeds_ui_format_options($targets);

  // Add unique and remove forms to mappings.
  $mappings = $feed->processor->getMappings();
  $form['unique_flags'] = $form['remove_flags'] = array(
    '#tree' => TRUE,
  );

  if (is_array($mappings)) {
    foreach ($mappings as $i => $mapping) {

      $mappings[$i]['source'] = isset($source_options) ? $source_options[$mappings[$i]['source']] : $mappings[$i]['source'];
      $mappings[$i]['target'] = $target_options[$mappings[$i]['target']];
      $param = array(
        'processor' => $feed->processor,
        'mapping' => $mapping,
      );

      if (isset($targets[$mapping['target']]['optional_unique']) && $targets[$mapping['target']]['optional_unique'] === TRUE) {

        $form['unique_flags'][$i] = array(
          '#type' => 'checkbox',
          '#default_value' => !empty($mapping['unique']),
          '#attributes' => array('class' => 'feeds-ui-trigger-submit'),
        );
      }
      $form['remove_flags'][$i] = array(
        '#type' => 'checkbox',
        '#title' => t('Remove'),
        '#prefix' => '<div class="feeds-ui-checkbox-link">',
        '#suffix' => '</div>',
      );
    }
  }

  $form['#mappings'] = $mappings;
  $form['#targets'] = $targets;
  if ($sources) {
    $form['source'] = array(
      '#type' => 'select',
      '#options' => array('' => t('Select a source')) + $source_options,
    );
  }
  else {
    $form['source'] = array(
      '#type' => 'textfield',
      '#size' => 20,
      '#default_value' => t('Name of source field'),
      '#attributes' => array('class' => 'hide-text-on-focus'),
    );
  }
  $form['target'] = array(
    '#type' => 'select',
    '#options' => array('' => t('Select a target')) + _feeds_ui_format_options($targets),
  );
  $form['add'] = array(
    '#type' => 'submit',
    '#value' => t('Add'),
    '#submit' => array('feeds_ui_mapping_form_add_submit'),
  );
  $form['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#attributes' => array('class' => 'feeds-ui-mapping-save'),
  );
  return $form;
}

/**
 * Submit handler for add button on feeds_ui_mapping_form().
 */
function feeds_ui_mapping_form_add_submit($form, &$form_state) {
  $feed = $form['#feed'];
  try {
    $feed->processor->addMapping($form_state['values']['source'], $form_state['values']['target']);
    $feed->processor->save();
    drupal_set_message(t('Mapping has been added.'));
  }
  catch (Exception $e) {
    drupal_set_message($e->getMessage(), 'error');
  }
}

/**
 * Submit handler for save button on feeds_ui_mapping_form().
 */
function feeds_ui_mapping_form_submit($form, &$form_state) {
  if (!isset($form_state['values']['unique_flags'])) {
    return;
  }
  $processor = $form['#feed']->processor;
  $mappings = $processor->config['mappings'];
  // We may set some unique flags here that we remove in the subsequent step.
  // That's fine.
  foreach ($form_state['values']['unique_flags'] as $k => $v) {
    $processor->setUnique($mappings[$k]['source'], $mappings[$k]['target'], $v);
  }
  foreach ($form_state['values']['remove_flags'] as $k => $v) {
    if ($v) {
      $processor->removeMapping($mappings[$k]['source'], $mappings[$k]['target']);
    }
  }
  drupal_set_message(t('Your changes have been saved.'));
  $processor->save();
}

/**
 * Walk the result of FeedsParser::getMappingSources() or
 * FeedsProcessor::getMappingTargets() and format them into
 * a Form API options array.
 */
function _feeds_ui_format_options($options) {
  $result = array();
  foreach ($options as $k => $v) {
    if (is_array($v) && !empty($v['name'])) {
      $result[$k] = $v['name'];
    }
    elseif (is_array($v)) {
      $result[$k] = $k;
    }
    else {
      $result[$k] = $v;
    }
  }
  return $result;
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
 *     'body' => 'the body of the container, may also be an array of more
 *                containers.',
 *     'class' => 'the class of the container.',
 *     'id' => 'the id of the container',
 *   );
 */
function theme_feeds_ui_container($container) {

  $class = empty($container['class']) ? ' plain': " {$container['class']}";
  $id = empty($container['id']) ? '': ' id="'. $container['id'] .'"';
  $output = '<div class="feeds-container'. $class .'"'. $id .'>';

  if (isset($container['actions']) && count($container['actions'])) {
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
    t('Unique target'),
    '&nbsp;',
  );
  $rows = array();
  if (is_array($form['#mappings'])) {
    foreach ($form['#mappings'] as $i => $mapping) {
      $rows[] = array(
        $mapping['source'],
        $mapping['target'],
        drupal_render($form['unique_flags'][$i]),
        drupal_render($form['remove_flags'][$i]),
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
  $output = '<div class="help feeds-admin-ui""'. drupal_render($form['help']) .'</div>';
  $output .= theme('table', $header, $rows);
  $output .= drupal_render($form);
  return $output;
}