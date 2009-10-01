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
   * Fetch content from a source and return it.
   * 
   * Stub method. Every class that extends FeedsFetcher must implement this method.
   *
   * @param $source 
   *   Source value as entered by user through sourceForm().
   * 
   * @todo: Define format of return value?
   */
  public abstract function fetch($source);
}
