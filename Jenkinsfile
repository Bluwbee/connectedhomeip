pipeline
{
    agent { dockerfile true }
    
    stages
    {
        stage("Building")
        {
            environment{
                DEBIAN_FRONTEND = 'noninteractive'
                ZAP_DEVELOPMENT_PATH = "${env.WORKSPACE}/zap-dir"
                CYPRESS_CACHE_FOLDER = "${WORKSPACE}/.cypress/.cache"
            }
            steps
            {
                echo "hello from docker"
                
                // Download and install ZAP
                dir('zap-dir'){
                    checkout scmGit(branches: [[name: 'refs/tags/v2023.10.30-nightly']], extensions: [], userRemoteConfigs: [[credentialsId: '0290ee72-6c30-41a5-9d3a-ff55551eb0b8', url: 'https://github.com/project-chip/zap.git']])
                    
                    sh '. ./src-script/install-packages-ubuntu' 
            
                    sh 'npm install -g n'
                    sh '''#!/bin/bash
                        n 19.0.0
                    '''
                    sh 'node --version'
                    
                    sh 'ZAP_DEVELOPMENT_PATH=${PWD}/zap'
                    sh 'ls ${ZAP_DEVELOPMENT_PATH}'
                    
                    sh 'export CYPRESS_CACHE_FOLDER=${PWD}/cypress/.cache'
                }
                
                sh 'git config --system --add safe.directory "*"'

                sh '''#!/bin/bash
                    source scripts/tools/zap/zap_bootstrap.sh
                '''
                
                dir('examples/air-quality-sensor-app/linux'){

                    sh 'git submodule update --init'

                    sh '''#!/bin/bash
                        source ../../../scripts/activate.sh
                    '''
            
                    sh '''#!/bin/bash
                        gn gen out/debug
                    '''
                    sh '''#!/bin/bash
                        ninja -C out/debug
                    '''
                }
            }
        }
    }
}

