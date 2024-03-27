pipeline
{
    agent { 
        docker { 
            image "ubuntu:latest" 
            args  "-u 0"
        } 
    }
    
    stages
    {
        stage("Building")
        {
            environment{
                DEBIAN_FRONTEND = 'noninteractive'
                ZAP_DEVELOPMENT_PATH = "${env.WORKSPACE}/zap-dir"
            }
            steps
            {
                echo "hello from docker"
                
                sh 'apt-get update'
                sh 'apt-get install -y gn git gcc g++ python3 pkg-config libssl-dev libdbus-1-dev libglib2.0-dev ninja-build python3-venv python3-dev unzip'
                sh 'apt-get install -y libavahi-client-dev python3-pip libgirepository1.0-dev libcairo2-dev libreadline-dev libsdl2-dev'
                sh 'apt-get install -y npm wget'
                
                // Download and install ZAP
                dir('zap-dir'){
                    checkout scmGit(branches: [[name: 'refs/tags/v2023.10.30-nightly']], extensions: [], userRemoteConfigs: [[credentialsId: '0290ee72-6c30-41a5-9d3a-ff55551eb0b8', url: 'https://github.com/project-chip/zap.git']])
                    
                    sh '. ./src-script/install-packages-ubuntu' 
            
                    sh 'npm install -g n'
                    sh '''#!/bin/bash
                        n 19.0.0
                    '''
                    sh 'node --version'
                }
                
                sh 'git config --system --add safe.directory "*"'
                sh 'git submodule update --init'
                
                sh '''#!/bin/bash
                    scripts/tools/zap/zap_bootstrap.sh
                '''
                sh '''#!/bin/bash
                    source ./scripts/activate.sh
                ''' 
    
                // Reinstall the troublesome module + install missing
                sh 'python3 -m pip uninstall prompt_toolkit'
                sh 'python3 -m pip install prompt_toolkit click lark jinja2 stringcase'
            
                sh 'gn gen examples/air-quality-sensor-app/linux/out/debug'
                sh 'ninja -C examples/air-quality-sensor-app/linux/out/debug' 
            }
        }
    }
}

