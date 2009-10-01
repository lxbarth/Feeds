<?php
// $Id$

/**
 * Abstract class, defines interface for fetchers.
 * 
 * Not using interfaces because we need a simple inheritence tree for determining the 
 * plugin type. See hook_feeds_plugin().
 */
abstract class FeedsFetcher extends FeedsConfigurable {

  /**
   * Source form.
   */
  public function sourceForm(&$form_state) {
    $form = array();
    $form['source'] = array(
      '#type' => 'textfield',
      '#title' => t('URL'),
      '#description' => t('Enter the URL for this feed.'),
      '#default_value' => $this->config['source'],
    );
    return $form;
  }

  /**
   * Validation handler for sourceForm().
   */
  public function sourceFormValidate($form, &$form_state) {
  }

  /**
   * Submit  handler for sourceForm().
   * 
   * Delete existing source definitions and write new one.
   */
  public function sourceFormSubmit($form, &$form_state) {
    // Nid is being set by feeds_form_alter() if form is being presented on a node form.
    if (empty($form_state['values']['nid'])) {
      db_query('DELETE FROM {feeds_source} WHERE id = "%s"');
    }
    else {
      db_query('DELETE FROM {feeds_source} WHERE id = "%s" AND nid = %d', $this->id, $form['values']['nid']);
    }
    $object = array(
      'id' => $this->id,
      'nid' => $form_state['values']['nid'], 
      'source' => $form_state['values']['source'],
    );
    drupal_write_record('feeds_source', $object);
  }

  /**
   * Fetch content from a source and return it.
   * 
   * Stub method. Every class that extends FeedsFetcher must implement this method.
   *
   * @param $source 
   *   Source value as entered by user through sourceForm().
   * 
   * @todo: Define format of return value?
   */
  public abstract function fetch($nid = NULL);
}
