<?php

/**
 * @file
 * Pubsubhubbub subscriber library.
 */

/**
 * Pubsub subscriber.
 */
class PuSHSubscriber {
  protected $domain;
  protected $subscriber_id;
  protected $subscription_class;
  protected $env;

  /**
   * Singleton.
   *
   * PuSHSubscriber identifies a unique subscription by a domain and a numeric
   * id. The numeric id is assumed to e unique in its domain.
   *
   * @param $domain
   *   A string that identifies the domain in which $subscriber_id is unique.
   * @param $subscriber_id
   *   A numeric subscriber id.
   * @param $subscription_class
   *   The class to use for handling subscriptions. Class MUST implement
   *   PuSHSubscriberSubscriptionInterface
   * @param PuSHSubscriberEnvironmentInterface $env
   *   Environmental object for messaging and logging.
   */
  public static function instance($domain, $subscriber_id, $subscription_class, PuSHSubscriberEnvironmentInterface $env) {
    static $subscribers;
    if (!isset($subscriber[$domain][$subscriber_id])) {
      $subscriber = new PuSHSubscriber($domain, $subscriber_id, $subscription_class, $env);
    }
    return $subscriber;
  }

  /**
   * Protect constructor.
   */
  protected function __construct($domain, $subscriber_id, $subscription_class, PuSHSubscriberEnvironmentInterface $env) {
    $this->domain = $domain;
    $this->subscriber_id = $subscriber_id;
    $this->subscription_class = $subscription_class;
    $this->env = $env;
  }

  /**
   * Subscribe to a given URL. Attempt to retrieve 'hub' and 'self' links from
   * document at $url and issue a subscription request to the hub.
   *
   * @param $url
   *   The URL of the feed to subscribe to.
   * @param $callback_url
   *   The full URL that hub should invoke for subscription verification or for
   *   notifications.
   * @param $hub
   *   The URL of a hub. If given overrides the hub URL found in the document
   *   at $url.
   */
  public function subscribe($url, $callback_url, $hub = '') {
    // Fetch document, find rel=hub and rel=self.
    // If present, issue subscription request.
    $request = curl_init($url);
    curl_setopt($request, CURLOPT_FOLLOWLOCATION, TRUE);
    curl_setopt($request, CURLOPT_RETURNTRANSFER, TRUE);
    $data = curl_exec($request);
    if (curl_getinfo($request, CURLINFO_HTTP_CODE) == 200) {
      $xml = new SimpleXMLElement($data);
      $xml->registerXPathNamespace('atom', 'http://www.w3.org/2005/Atom');
      if (empty($hub) && $hub = @current($xml->xpath("//atom:link[attribute::rel='hub']"))) {
        $hub = (string) $hub->attributes()->href;
      }
      if ($self = @current($xml->xpath("//atom:link[attribute::rel='self']"))) {
        $self = (string) $self->attributes()->href;
      }
    }
    curl_close($request);
    // Fall back to $url if $self is not given.
    if (!isset($self)) {
      $self = $url;
    }
    if (!empty($hub) && !empty($self)) {
      $this->request($hub, $self, 'subscribe', $callback_url);
    }
  }

  /**
   * @todo Unsubscribe from a hub.
   * @todo Make sure we unsubscribe with the correct topic URL as it can differ
   * from the initial subscription URL.
   *
   * @param $topic_url
   *   The URL of the topic to unsubscribe from.
   * @param $callback_url
   *   The callback to unsubscribe.
   */
  public function unsubscribe($topic_url, $callback_url) {
    if ($sub = $this->loadSubscription($topic_url, $callback_url)) {
      $sub->delete();
    }
  }

  /**
   * Issue a subscribe or unsubcribe request to a PubsubHubbub hub.
   *
   * @param $hub
   *   The URL of the hub's subscription endpoint.
   * @param $topic
   *   The topic URL of the feed to subscribe to.
   * @param $mode
   *   'subscribe' or 'unsubscribe'.
   * @param $callback_url
   *   The subscriber's notifications callback URL.
   *
   * Compare to http://pubsubhubbub.googlecode.com/svn/trunk/pubsubhubbub-core-0.2.html#anchor5
   *
   * @todo Make concurrency safe.
   */
  protected function request($hub, $topic, $mode, $callback_url) {
    $secret = hash('sha1', uniqid(rand(), true));
    $post_fields = array(
      'hub.callback' => $callback_url,
      'hub.mode' => $mode,
      'hub.topic' => $topic,
      'hub.verify' => 'sync',
      'hub.lease_seconds' => '', // Permanent subscription.
      'hub.secret' => $secret,
      'hub.verify_token' => md5(session_id() . rand()),
    );
    $sub = new $this->subscription_class($this->domain, $this->subscriber_id, $hub, $topic, $secret, $mode, $post_fields);
    $sub->save();
    // Issue subscription request.
    $request = curl_init($hub);
    curl_setopt($request, CURLOPT_POST, TRUE);
    curl_setopt($request, CURLOPT_POSTFIELDS, $post_fields);
    curl_setopt($request, CURLOPT_RETURNTRANSFER, TRUE);
    curl_exec($request);
    $code = curl_getinfo($request, CURLINFO_HTTP_CODE);
    if (!in_array($code, array(202, 204))) {
      $sub->status = $mode .' failed';
      $sub->save();
      $this->env->msg("Error subscribing to hub ($code)", 'error');
      $this->env->log("Error subscribing to hub ({$this->domain} / {$this->subscriber_id}, code $code)", 'error');
    }
    curl_close($request);
  }

  /**
   * Verify subscription request. After a hub has received a subscription
   * request (see PuSHSubscriber::request()) it sends back a challenge verifying
   * that a subscription indeed was requested ($_GET['hub_challenge']). This
   * method handles the challenge. Example:
   *
   * $subscriber->verifySubscriptionRequest($_GET['hub_challenge']).
   */
  public function verifySubscriptionRequest($verify_fields) {
    if ($sub = $this->loadSubscription($this->domain, $this->subscriber_id)) {
      if ($verify_fields['hub_verify_token'] == $sub['post_fields']['hub.verify_token']) {
        // Save and log subscription as verified, respond with success code.
        $sub->status = 'subscribed';
        $sub->save();
        $this->env->log("Verified subscription for {$this->domain} / {$this->subscriber_id}");
        header('HTTP/1.1 200 "Found"', null, 200);
        print $verify_fields['hub_challenge'];
        exit();
      }
    }
    header('HTTP/1.1 404 "Not Found"', null, 404);
    $this->env->log("Could not verify subscription for {$this->domain} / {$this->subscriber_id}", 'error');
    exit();
  }

  /**
   * Receive a notification.
   *
   * @return
   *   An XML string if the received notification is XML, FALSE otherwise.
   */
  public function receive() {
    if ($_SERVER['REQUEST_METHOD'] == 'POST') {
      $raw = file_get_contents('php://input');
      if (@simplexml_load_string($raw)) {
        return $raw;
      }
    }
    return FALSE;
  }

  /**
   * Helper for loading a subscription.
   */
  protected function loadSubscription($domain, $subscriber_id) {
    return call_user_func("{$this->subscription_class}::load", $domain, $subscriber_id);
  }
}

/**
 * Implement to provide a storage backend for subscriptions.
 *
 * Variables passed in to the constructor must be accessible as public class
 * variables.
 */
interface PuSHSubscriptionInterface {
  /**
   * @param $domain
   *   A string that defines the domain in which the subscriber_id is unique.
   * @param $subscriber_id
   *   A unique numeric subscriber id.
   * @param $hub
   *   The URL of the hub endpoint.
   * @param $topic
   *   The topic to subscribe to.
   * @param $secret
   *   A secret key used for message authentication.
   * @param $status
   *   The status of the subscription.
   *   'subscribe' - subscribing to a feed.
   *   'unsubscribe' - unsubscribing from a feed.
   *   'subscribed' - subscribed.
   *   'subscribe failed' - subscribe request failed.
   *   'unsubscribe failed' - unsubscribe request failed.
   * @param $post_fields
   *   An array of the fields posted to the hub.
   */
  public function __construct($domain, $subscriber_id, $hub, $topic, $secret, $status = '', $post_fields = '');

  /**
   * Save a subscription.
   */
  public function save();

  /**
   * Load a subscription.
   *
   * @return
   *   A PuSHSubscriptionInterface object if a subscription exist, NULL
   *   otherwise.
   */
  public static function load($domain, $subscriber_id);

  /**
   * Delete a subscription.
   */
  public function delete();
}

/**
 * Implement to provide environmental functionality like user messages and
 * logging.
 */
interface PuSHSubscriberEnvironmentInterface {
  /**
   * A message to be displayed to the user on the current page load.
   *
   * @param $msg
   *   A string that is the message to be displayed.
   * @param $level
   *   A string that is either 'status', 'warning' or 'error'.
   */
  public function msg($msg, $level = 'status');

  /**
   * A log message to be logged to the database or the file system.
   *
   * @param $msg
   *   A string that is the message to be displayed.
   * @param $level
   *   A string that is either 'status', 'warning' or 'error'.
   */
  public function log($msg, $level = 'status');
}