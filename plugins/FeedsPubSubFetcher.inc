<?php

feeds_include_library('PuSHSubscriber.inc', 'PuSHSubscriber');

/**
 * FeedsImportBatch for PubSub Fetcher.
 */
class FeedsPubSubBatch extends FeedsHTTPBatch {
  protected $raw;

  /**
   * Constructor
   *
   * @param $raw
   *   The raw content of the fat ping.
   */
  function __construct($raw) {
    $this->raw = $raw;
    parent::__construct();
  }

  /**
   * Override FeedsHTTPBatch::getRaw() and simply return the raw content
   * populated on instnatiation.
   */
  function getRaw() {
    return $this->raw;
  }
}

/**
 * Publish/Subscribe fetcher. Supports at the moment only PubSubHubbub (PuSH).
 */
class FeedsPubSubFetcher extends FeedsHTTPFetcher {

  /**
   * Implementation of FeedsFetcher::fetch().
   */
  public function fetch(FeedsSource $source) {
    $source_config = $source->getConfigFor($this);
    // Handle fat ping if present, otherwise pass up to HTTP fetcher.
    if ($raw = feeds_push_subscriber($this->id, $source->feed_nid)->receive()) {
      return new FeedsPubSubBatch($raw);
    }
    return parent::fetch($source);
  }

  /**
   * Implementation of FeedsFetcher::request().
   */
  public function request($feed_nid = 0) {
    feeds_log($_GET);
    @feeds_log(file_get_contents('php://input'));
    // A subscription verification has been sent, verify.
    if ($_GET['hub_challenge']) {
      $this->pushVerifyRequest($feed_nid);
    }
    // No subscription notification has ben sent, we are being notified.
    else {
      try {
        feeds_source($this->id, $feed_nid)->existing()->import();
      }
      catch (Exception $e) {
        // In case of an error, respond with a 503 Service (temporary) unavailable.
        header('HTTP/1.1 503 "Not Found"', null, 503);
        exit();
      }
    }
  }

  /**
   * Override sourceSave() - subscribe to hub.
   */
  public function sourceSave(FeedsSource $source) {
    $this->subscribe($source);
  }

  /**
   * Override sourceDelete() - unsubscribe from hub.
   */
  public function sourceDelete(FeedsSource $source) {
    $this->unsubscribe($source);
  }

  /**
   * Implement FeedsFetcher::subscribe() - subscribe to hub.
   *
   * @todo Abstract out KeyAuth integration, this is not at all pub/sub
   * specific.
   */
  public function subscribe(FeedsSource $source) {
    $source_config = $source->getConfigFor($this);
    feeds_push_subscriber($this->id, $source->feed_nid)->subscribe($source_config['source'], url($this->path($source->feed_nid), array('absolute' => TRUE)), valid_url($this->config['designated_hub']) ? $this->config['designated_hub'] : '');
  }

  /**
   * Implement FeedsFetcher::unsubscribe() - unsubscribe from hub.
   */
  public function unsubscribe(FeedsSource $source) {
    $source_config = $source->getConfigFor($this);
    feeds_push_subscriber($this->id, $source->feed_nid)->unsubscribe($source_config['source'], url($this->path($source->feed_nid), array('absolute' => TRUE)));
  }

  /**
   * Implement FeedsFetcher::subscriptionPeriod().
   * Indicate how often a subscription needs to be renewed.
   *
   * @todo subscription_period should actually be retrieved from the hub's
   *   response to a subscription. This will mean a different subscription
   *   period per source, hence a major change to FeedsScheduler which is
   *   currently assuming a fixed period per task!
   */
  public function subscriptionPeriod() {
    return $this->config['subscription_period'];
  }

  /**
   * Verify a PubSubHubbub subscription request.
   */
  public function pushVerifyRequest($feed_nid) {
    feeds_push_subscriber($this->id, $feed_nid)->verifyRequest();
  }

  /**
   * Return defaults for configuration.
   */
  public function configDefaults() {
    $defaults = parent::configDefaults();
    return $defaults + array(
      'subscription_period' => 3600*24, // Renew subscription in 24 hours.
      'designated_hub' => '',
    );
  }

  /**
   * Override parent::configForm().
   */
  public function configForm(&$form_state) {
    $form = parent::configForm($form_state);
    $period = drupal_map_assoc(array(0, 900, 1800, 3600, 10800, 21600, 43200, 86400, 259200, 604800, 2419200), 'format_interval');
    $period[FEEDS_SCHEDULE_NEVER] = t('Never renew');
    $period[0] = t('Renew as often as possible');
    $form['subscription_period'] = array(
      '#type' => 'select',
      '#title' => t('Renew subscription after'),
      '#options' => $period,
      '#description' => t('This is the minimum time that must elapse before a subscription is renewed.'),
      '#default_value' => $this->config['subscription_period'],
    );
    $form['designated_hub'] = array(
      '#type' => 'textfield',
      '#title' => t('Designated hub'),
      '#description' => t('Enter the callback URL of a designated hub. If given, this hub will be used instead of the hub specified in the feed source.'),
      '#default_value' => $this->config['designated_hub'],
    );
    return $form;
  }
}

/**
 * Create a PubSubHubbub subscriber.
 *
 * @return PushSubscriber
 *   A PushSubscriber object.
 */
function feeds_push_subscriber($id, $subscriber_id) {
  return PushSubscriber::instance($id, $subscriber_id, 'PuSHSubscription', PuSHEnvironment::instance());
}

/**
 * Implement a PuSHSubscriptionInterface.
 */
class PuSHSubscription implements PuSHSubscriptionInterface {
  public $domain;
  public $subscriber_id;
  public $hub;
  public $topic;
  public $status;
  public $secret;
  public $post_fields;
  public $timestamp;

  /**
   * Load a subscription.
   */
  public static function load($domain, $subscriber_id) {
    if ($v = db_fetch_array(db_query("SELECT * FROM {feeds_push_subscriptions} WHERE domain = '%s' AND subscriber_id = %d", $domain, $subscriber_id))) {
      $v['post_fields'] = unserialize($v['post_fields']);
      return new PuSHSubscription($v['domain'], $v['subscriber_id'], $v['hub'], $v['topic'], $v['secret'], $v['status'], $v['post_fields'], $v['timestamp']);
    }
  }

  /**
   * Create a subscription.
   */
  public function __construct($domain, $subscriber_id, $hub, $topic, $secret, $status = '', $post_fields = '') {
    $this->domain = $domain;
    $this->subscriber_id = $subscriber_id;
    $this->hub = $hub;
    $this->topic = $topic;
    $this->status = $status;
    $this->secret = $secret;
    $this->post_fields = $post_fields;
  }

  /**
   * Save a subscription.
   */
  public function save() {
    $this->timestamp = time();
    $this->delete($this->domain, $this->subscriber_id);
    drupal_write_record('feeds_push_subscriptions', $this);
  }

  /**
   * Delete a subscription.
   */
  public function delete() {
    db_query("DELETE FROM {feeds_push_subscriptions} WHERE domain = '%s' AND subscriber_id = %d", $this->domain, $this->subscriber_id);
  }
}

/**
 * Provide environmental functions to the PuSHSubscriber library.
 */
class PuSHEnvironment implements PuSHSubscriberEnvironmentInterface {
  /**
   * Singleton.
   */
  public static function instance() {
    static $env;
    if (empty($env)) {
      $env = new PuSHEnvironment();
    }
    return $env;
  }

  /**
   * Implementation of PuSHSubscriberEnvironmentInterface::msg().
   */
  public function msg($msg, $level = 'status') {
    drupal_set_message($msg, $level);
  }

  /**
   * Implementation of PuSHSubscriberEnvironmentInterface::log().
   */
  public function log($msg, $level = 'status') {
    switch ($level) {
      case 'error':
        $severity = WATCHDOG_ERROR;
        break;
      case 'warning':
        $severity = WATCHDOG_WARNING;
        break;
      default:
        $severity = WATCHDOG_NOTICE;
        break;
    }
    feeds_log($msg);
    watchdog('FeedsPubSubFetcher', $msg, array(), $severity);
  }
}
