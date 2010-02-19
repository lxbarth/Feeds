<?php
// $Id: FeedsFetcher.inc,v 1.4 2009/12/20 23:54:44 alexb Exp $

/**
 * Abstract class, defines shared functionality between fetchers.
 *
 * Implements FeedsSourceInfoInterface to expose source forms to Feeds.
 */
abstract class FeedsFetcher extends FeedsPlugin {

  /**
   * Fetch content from a source and return it.
   *
   * Every class that extends FeedsFetcher must implement this method.
   *
   * @param $source
   *   Source value as entered by user through sourceForm().
   */
  public abstract function fetch(FeedsSource $source);

  /**
   * Clear all caches for results for given source.
   *
   * @param FeedsSource $source
   *   Source information for this expiry. Implementers can choose to only clear
   *   caches pertaining to this source.
   */
  public function clear(FeedsSource $source) {}

  /**
   * Request handler invoked if callback URL is requested. Locked down by
   * default. For an example use see FeedsPubSubFetcher.
   */
  public function request($feed_nid = 0) {
    return drupal_access_denied();
  }

  /**
   * Return callback path for this fetcher/source combination.
   *
   * @return
   *   Path for this fetcher/source combination. Requests to this path will be
   *   routed to FeedsFetcher::request(). The
   */
  public function path($feed_nid = 0) {
    if ($feed_nid) {
      return urlencode('feeds/fetcher/'. $this->id .'/'. $feed_nid);
    }
    return urlencode('feeds/fetcher/'. $this->id);
  }

  /**
   * Menu item definition for fetchers of this class. Note how the path
   * component in the item definition matches the return value of
   * FeedsFetcher::path();
   */
  public function menuItem() {
    return array(
      'feeds/fetcher/%feeds_importer' => array(
        'page callback' => 'feeds_fetcher_callback',
        'page arguments' => array(3, 4),
        'access callback' => TRUE,
        'file' => 'feeds.pages.inc',
        'type' => MENU_CALLBACK,
        ),
      );
  }

  /**
   * Subscribe to a source. Only implement if fetcher requires subscription.
   *
   * @param FeedsSource $source
   *   Source information for this subscription.
   */
  public function subscribe(FeedsSource $source) {}

  /**
   * Unsubscribe from a source. Only implement if fetcher requires subscription.
   *
   * @param FeedsSource $source
   *   Source information for unsubscribing.
   */
  public function unsubscribe(FeedsSource $source) {}

  /**
   * Indicate the time within which a subscription needs to be renewed.
   */
  public function subscriptionPeriod() {
    return FEEDS_SCHEDULE_NEVER;
  }
}
