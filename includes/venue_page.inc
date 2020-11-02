<?php

function rbge_cal_taxonomy_term_venue_page($term){
    
    $page_render = taxonomy_term_show($term);
    $page_render = $page_render['taxonomy_terms'][$term->tid];

    
    // load the whole venue tree
    $start_date = new DateTime();
    $end_date = new DateTime();
    $end_date->add(new DateInterval('P7D'));
    $period = array();
    $period['start'] = $start_date->format(DATE_ATOM);
    $period['start_timestamp'] = $start_date->getTimestamp();
    $period['end'] = $end_date->format(DATE_ATOM);
    $period['end_timestamp'] = $end_date->getTimestamp();
    $period['duration'] =  $period['end_timestamp']  - $period['start_timestamp'];
    module_load_include('inc', 'rbge_cal', 'includes/venue_event_tree');
    $venue_tree = get_venue_event_tree($period);
    $venue_data = $venue_tree[$term->tid];
    
    // wrap the map
    $page_render['field_geo_location']['#prefix'] = '<div class="rbge-float-right">';
    $page_render['field_geo_location']['#suffix'] = '</div>';
    
    // parent
    if(isset($venue_data->parents)){
        
        $page_render['paps_header']['#type'] = 'markup';
        $page_render['paps_header']['#markup'] = '<h2>Location</h2>';
        
        $parents = taxonomy_get_parents_all($venue_data->tid);
        $parents = array_reverse($parents);
        
        $page_render['parents']['#theme'] = 'item_list';
        $page_render['parents']['#attributes']['class'][] = 'rbge-cal-venue-location';
        
        foreach($parents as $parent){
            
            if(isset($page_render['parents']['#items'])) $arrow = ' &gt; ';
            else $arrow = '';
            
            if($parent->tid != $venue_data->tid){
                $page_render['parents']['#items'][] = $arrow . l($parent->name, 'taxonomy/term/' . $parent->tid);
            }else{
                $page_render['parents']['#items'][] = $arrow . $parent->name;
            }            
            
        }
        
    }

    // children
    if(isset($venue_data->children)){
        $page_render['kids_header']['#type'] = 'markup';
        $page_render['kids_header']['#markup'] = '<h2>Sub-Venues</h2>';
        $page_render['subs']['#theme'] = 'item_list';
        foreach($venue_data->children as $child){
            if(isset($child->events)){
                $event_count = count($child->events);
            }else{
                $event_count = 0;
            }
            $page_render['subs']['#items'][] = l($child->name . ' ~ ' . $event_count . ' events', 'taxonomy/term/' . $child->tid);
        }
    }
   
    // add the events for this venue
    
    $page_render['events_header']['#type'] = 'markup';
    $page_render['events_header']['#markup'] = '<h2>Events in next 7 days</h2>';
    $page_render['events']['#theme'] = 'item_list';
    
    if(isset($venue_data->events)){
        // add the events for this venue
        foreach($venue_data->events as $hit){

            // we arrange them by start date
            $start = new DateTime($hit[1]->start);
            $end = new DateTime($hit[1]->end);

            // build our own render array for the event - with nice time stamp.
            $event = entity_metadata_wrapper('node', $hit[0]);
            $page_render['events']['#items'][] = l($start->format("l jS F Y H:i - ") . $end->format("H:i") . ' ~ ' . $event->label(), 'node/' . $event->getIdentifier());


//            $page_render['events'][] = node_view($hit[0], 'teaser');
        }
    }else{
        // add the events for this venue
       $page_render['events_none']['#type'] = 'markup';
       $page_render['events_none']['#markup'] = '<p>No events are scheduled for this venue today</p>';        
    }
    

    // set the weights up so everything is in the right order
    unset($page_render['#weight']);
    $page_render['field_venue_manager']['#weight'] = -2;
    $page_render['field_geo_location']['#weight'] = -1;  
    $page_render['description']['#weight'] = 30;
    $page_render['paps_header']['#weight'] = 40;
    $page_render['parents']['#weight'] = 50;
    $page_render['events_header']['#weight'] = 60;
    $page_render['events']['#weight'] = 70;
    $page_render['events_none']['#weight'] = 70;
    $page_render['kids_header']['#weight'] = 80;
    $page_render['subs']['#weight'] = 90;
    
    $page_render['#sorted'] = false;
    
    //dpm($page_render);

    
    return $page_render;
}

