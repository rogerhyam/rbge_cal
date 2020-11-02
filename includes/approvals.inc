<?php

    /*
        handles approval of events by categories and venues without need for login
    */
    function rbge_cal_approvals($thing, $action, $key){
        
        // find the event they are talking about
        $query = new EntityFieldQuery();

        $query->entityCondition('entity_type', 'node')
          ->entityCondition('bundle', 'event')
          ->fieldCondition('field_'. $thing .'_approval_key', 'value', $key, '=');
//          ->addMetaData('account', user_load(1)); // Run the query as user 1.

        $result = $query->execute();

        if (isset($result['node'])) {
          $event_nids = array_keys($result['node']);
          $events = entity_load('node', $event_nids);
        }
        
        if(!isset($events) || count($events) < 1){
            drupal_set_message("No event found for the key $key. Perhaps the URL is old or broken?");
            return "Not Found";
        }
     
        if(count($events) > 1){
            drupal_set_message("Multiple events found for key $key. This must be an error. Please report it.");
            return "Error";
        }
           
        $event_nids = array_keys($events);
        $event = $wrapper = entity_metadata_wrapper('node', $events[$event_nids[0]]);
        
        if($action == 'view'){
            drupal_goto('node/' . $event_nids[0]);
        }else{
            // if they have the key then we assume they have permission and we just do the business
            if($thing == 'category'){
              $event->field_category_manager_approved->set(1);
              drupal_set_message("Category Manager Approval has been given to this event");
            } 
            if($thing == 'venue'){
                $event->field_venue_manager_approved->set(1);
                drupal_set_message("Venue Manager Approval has been given to this event");
            } 
            $event->save();
            
            drupal_goto('node/' . $event_nids[0]);
            
        }
        //https://cal-dev.rbge.info/rbge/apps/venue/view/56a85bba1bcaf
    }

?>